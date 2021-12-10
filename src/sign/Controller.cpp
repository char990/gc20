#include <sign/Controller.h>
#include <sign/GroupVms.h>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>
#include <module/MyDbg.h>
#include <module/ptcpp.h>
#include <gpio/GpioOut.h>
#include <module/DS3231.h>
#include <layer/LayerNTS.h>

Controller::Controller()
    : db(DbHelper::Instance()), groups(DbHelper::Instance().GetUciProd().NumberOfGroups())
{
    pMainPwr = new GpioIn(10, 10, PIN_MAIN_FAILURE);
    pBatLow = new GpioIn(10, 10, PIN_BATTERY_LOW);
    pBatOpen = new GpioIn(10, 10, PIN_BATTERY_OPEN);
    for (int i = 0; i < 4; i++)
    {
        extInput.at(i) = new GpioIn(2, 2, EXTIN_PINS[i]);
        extInput.at(i)->Init(Utils::STATE5::S5_1);
    }
    groups.assign(groups.size(), nullptr);

    ctrllerError.SetV(db.GetUciProcess().CtrllerErr());
    overtempFault.SetCNT(3); // set as 3 minutes
    overtempFault.SetState(ctrllerError.IsSet(DEV::ERROR::EquipmentOverTemperature));
    long dt = db.GetUciUser().DisplayTimeout();
    if (dt == 0)
    {
        displayTimeout.Clear();
    }
    else
    {
        displayTimeout.Setms(dt * 60000);
    }

    UciProd &prod = db.GetUciProd();
    switch (prod.ProdType())
    {
    case PRODUCT::VMS:
        for (int i = 0; i < groups.size(); i++)
        {
            groups.at(i) = new GroupVms(i + 1);
        }
        break;
    case PRODUCT::ISLUS:
        for (int i = 0; i < groups.size(); i++)
        {
            groups.at(i) = new GroupIslus(i + 1);
        }
        break;
    default:
        MyThrow("Unknown ProdType:%d", static_cast<int>(prod.ProdType()));
        break;
    }
}

Controller::~Controller()
{
    for (auto &ei : extInput)
    {
        delete ei;
    }
    delete pBatOpen;
    delete pBatLow;
    delete pMainPwr;
    for (auto &g : groups)
    {
        delete g;
    }
    if (tmrEvt != nullptr)
    {
        tmrEvt->Remove(this);
    }
}

void Controller::Init(TimerEvent *tmrEvt_)
{
    if (tmrEvt != nullptr)
    {
        MyThrow("Re-invoke Controller::Init()");
    }
    tmrEvt = tmrEvt_;
    tmrEvt->Add(this);
}

void Controller::SetTcpServer(TcpServer *tcpServer)
{
    this->tcpServer = tcpServer;
}

void Controller::PeriodicRun()
{
    // run every CTRLLER_TICK
    if (rr_flag != 0)
    {
        if (rrTmr.IsExpired())
        {
            auto &evt = DbHelper::Instance().GetUciEvent();
            if (rr_flag & RQST_NETWORK)
            {
                tcpServer->Close();
                evt.Push(0, "ETH1 restart");
                PrintDbg(DBG_LOG, "ETH1 restart...\n");
                system("ifdown ETH1");
                system("ifup ETH1");
                PrintDbg(DBG_LOG, "Done.\n");
                tcpServer->Open();
            }
            if (rr_flag & RQST_REBOOT)
            {
                const char *_re = " -> -> -> reboot";
                evt.Push(0, _re);
                PrintDbg(DBG_LOG, "\n%s...\n", _re);
                MyThrow("\n%s...\n", _re);
            }
            if (rr_flag & RQST_RESTART)
            {
                const char *_re = " -> -> -> restart";
                evt.Push(0, _re);
                PrintDbg(DBG_LOG, "\n%s...\n", _re);
                MyThrow("\n%s...\n", _re);
            }
            rr_flag = 0;
            rrTmr.Clear();
        }
    }

    for (auto &g : groups)
    {
        g->PeriodicRun();
    }

    if (++cnt100ms >= CTRLLER_MS(100))
    { // 100ms
        cnt100ms = 0;
        //ExtInputFunc();
        PowerMonitor();
        if (displayTimeout.IsExpired())
        {
            if (!IsPlnActive(0))
            {
                ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 1);
                for (auto &g : groups)
                {
                    g->DispFrm(0);
                }
            }
            displayTimeout.Clear();
        }
        if (sessionTimeout.IsExpired())
        {
            if (LayerNTS::IsAnySessionTimeout())
            {
                LayerNTS::ClearAllSessionTimeout();
                ctrllerError.Push(DEV::ERROR::CommunicationsTimeoutError, 1);
            }
            sessionTimeout.Clear();
        }
    }

    if (++msTemp >= CTRLLER_MS(60 * 1000))
    { // every 60s
        msTemp = 0;
        maxTemp = 0;
        curTemp = 0;
        int t;
        if (pDS3231->GetTemp(&t) == 0)
        {
            curTemp = t;
            if (curTemp > maxTemp)
            {
                maxTemp = curTemp;
            }
            auto ot = DbHelper::Instance().GetUciUser().OverTemp();
            if (curTemp > ot)
            {
                overtempFault.Check(1);
                if (!ctrllerError.IsSet(DEV::ERROR::EquipmentOverTemperature) && overtempFault.IsHigh())
                {
                    ctrllerError.Push(DEV::ERROR::EquipmentOverTemperature, true);
                    DbHelper::Instance().GetUciAlarm().Push(0, "Controller OverTemperatureAlarm ONSET: %d'C", curTemp);
                    overtempFault.ClearEdge();
                }
            }
            else if (curTemp < (ot - 3))
            {
                overtempFault.Check(0);
                if (ctrllerError.IsSet(DEV::ERROR::EquipmentOverTemperature) && overtempFault.IsLow())
                {
                    ctrllerError.Push(DEV::ERROR::EquipmentOverTemperature, false);
                    DbHelper::Instance().GetUciAlarm().Push(0, "Controller OverTemperatureAlarm CLEAR: %d'C", curTemp);
                    overtempFault.ClearEdge();
                }
            }
        }
    }
}

void Controller::PowerMonitor()
{ // 100ms periodic
    pMainPwr->PeriodicRun();
    pBatLow->PeriodicRun();
    pBatOpen->PeriodicRun();
    if (pMainPwr->HasEdge())
    {
        pMainPwr->ClearEdge();
        ctrllerError.Push(DEV::ERROR::PowerFailure, pMainPwr->IsLow());
    }
    if (pBatOpen->HasEdge())
    {
        pBatOpen->ClearEdge();
        ctrllerError.Push(DEV::ERROR::BatteryFailure, pBatOpen->IsLow());
        if (pBatOpen->IsLow()) // if there is BatOpen, don not check BatLow
        {
            return;
        }
    }
    if (pBatLow->HasEdge())
    {
        pBatLow->ClearEdge();
        ctrllerError.Push(DEV::ERROR::BatteryLow, pBatLow->IsLow());
    }
}

void Controller::ExtInputFunc()
{
    for (int i = 0; i < 4; i++)
    {
        auto gin = extInput.at(i);
        gin->PeriodicRun();
        if (gin->IsRising())
        {
            gin->ClearRising();
            uint8_t msg = i + 3;
            DbHelper::Instance().GetUciEvent().Push(0, "Leading edge of external input[%d]", i + 1);
            for (auto &g : groups)
            {
                g->DispExt(msg);
            }
            // only the highest priority will be triggered, ignore others
            while (++i < 4)
            { // clear others changed flag
                gin = extInput.at(i);
                gin->PeriodicRun();
                gin->ClearRising();
            };
            return;
        }
    }
}

void Controller::RefreshDispTime()
{
    long ms = DbHelper::Instance().GetUciUser().DisplayTimeout();
    if (ms > 0)
    {
        displayTimeout.Setms(ms * 60000);
        ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 0);
    }
}

void Controller::RefreshSessionTime()
{
    long ms = DbHelper::Instance().GetUciUser().SessionTimeout();
    if (ms == 0)
    {
        sessionTimeout.Clear();
    }
    else
    {
        sessionTimeout.Setms(ms * 1000);
    }
    ctrllerError.Push(DEV::ERROR::CommunicationsTimeoutError, 0);
}

bool Controller::IsFrmActive(uint8_t id)
{
    if (id > 0)
    {
        for (auto &g : groups)
        {
            if (g->IsFrmActive(id))
            {
                return true;
            }
        }
    }
    return false;
}

bool Controller::IsMsgActive(uint8_t id)
{
    if (id > 0)
    {
        for (auto &g : groups)
        {
            if (g->IsMsgActive(id))
            {
                return true;
            }
        }
    }
    return false;
}

bool Controller::IsPlnActive(uint8_t id)
{
    for (auto &g : groups)
    {
        if (g->IsPlanActive(id))
        {
            return true;
        }
    }
    return false;
}

bool Controller::IsPlnEnabled(uint8_t id)
{
    for (auto &g : groups)
    {
        if (g->IsPlanEnabled(id))
        {
            return true;
        }
    }
    return false;
}

APP::ERROR Controller::CmdSystemReset(uint8_t *cmd)
{
    auto grpId = cmd[1];
    auto lvl = cmd[2];
    if (grpId > groups.size())
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (grpId != 0)
    {
        GetGroup(grpId)->SystemReset(lvl);
    }
    else
    {
        for (auto &g : groups)
        {
            g->SystemReset(lvl);
        }
        auto &db = DbHelper::Instance();
        if (lvl >= 2)
        {
            db.GetUciFault().Reset();
            overtempFault.SetState(false);
            ctrllerError.Clear();
            db.GetUciProcess().SaveCtrllerErr(ctrllerError.GetV());
        }
        if (lvl >= 3)
        {
            if(db.GetUciProd().ProdType() == PRODUCT::VMS)
            {   // Clearing all frame/msg is for VMS only
                // ISLUS's frame/msg/plan can not be changed
                db.GetUciFrm().Reset();
                db.GetUciMsg().Reset();
            }
            db.GetUciPln().Reset();
        }
        if (lvl == 255)
        {
            db.GetUciUser().LoadFactoryDefault();
        }
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdDispFrm(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t frmId = cmd[2];
    if (grpId == 0 || grpId > groups.size())
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (frmId != 0)
    {
        if (!DbHelper::Instance().GetUciFrm().IsFrmDefined(frmId))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        // reject frames
        auto signs = GetGroup(grpId)->GetSigns();
        for(auto & sign : signs)
        {
            if(db.GetUciProd().GetSignCfg(sign->SignId()).rjctFrm.GetBit(frmId))
            {
                return APP::ERROR::SyntaxError;
            }
        }
    }
    return GetGroup(grpId)->DispFrm(frmId);
}

APP::ERROR Controller::CmdDispMsg(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t msgId = cmd[2];
    if (grpId == 0 || grpId > Controller::Instance().GroupCnt())
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    Message *msg;
    if (msgId != 0)
    {
        if (!DbHelper::Instance().GetUciMsg().IsMsgDefined(msgId))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
    }
    return GetGroup(grpId)->DispMsg(msgId);
}

APP::ERROR Controller::CmdDispAtomicFrm(uint8_t *cmd, int len)
{
    uint8_t grpId = cmd[1];
    if (grpId == 0 || grpId > groups.size())
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    return GetGroup(grpId)->DispAtomicFrm(cmd);
}

APP::ERROR Controller::CmdEnDisPlan(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t plnId = cmd[2];
    APP::ERROR r = APP::ERROR::AppNoError;
    if (grpId > groups.size() || grpId == 0)
    {
        r = APP::ERROR::UndefinedDeviceNumber;
    }
    else if (plnId != 0 && !DbHelper::Instance().GetUciPln().IsPlnDefined(plnId))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else
    {
        r = GetGroup(grpId)->EnDisPlan(plnId, cmd[0] == static_cast<uint8_t>(MI::CODE::EnablePlan));
    }
    return r;
}

APP::ERROR Controller::CmdSetDimmingLevel(uint8_t *cmd)
{
    uint8_t entry = cmd[1];
    uint8_t *p;
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] > groups.size())
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        if (p[1] != 0 && (p[2] == 0 || p[2] > 16))
        {
            return APP::ERROR::DimmingLevelNotSupported;
        }
        p += 3;
    }
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        uint8_t dimming = (p[1] == 0) ? 0 : p[2];
        if (p[0] == 0)
        {
            for (auto &g : groups)
            {
                g->SetDimming(dimming);
            }
        }
        else
        {
            GetGroup(p[0])->SetDimming(dimming);
        }
        p += 3;
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdPowerOnOff(uint8_t *cmd, int len)
{
    uint8_t entry = cmd[1];
    uint8_t *p;
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] > groups.size())
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        if (p[1] > 1)
        {
            return APP::ERROR::SyntaxError;
        }
        p += 2;
    }
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] == 0)
        {
            for (auto &g : groups)
            {
                g->SetPower(p[1]);
            }
        }
        else
        {
            GetGroup(p[0])->SetPower(p[1]);
        }
        p += 2;
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdDisableEnableDevice(uint8_t *cmd, int len)
{
    uint8_t entry = cmd[1];
    uint8_t *p;
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] > groups.size())
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        if (p[1] > 1)
        {
            return APP::ERROR::SyntaxError;
        }
        p += 2;
    }
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] == 0)
        {
            for (auto &g : groups)
            {
                g->SetDevice(p[1]);
            }
        }
        else
        {
            GetGroup(p[0])->SetDevice(p[1]);
        }
        p += 2;
    }
    return APP::ERROR::AppNoError;
}

int Controller::CmdRequestEnabledPlans(uint8_t *txbuf)
{
    txbuf[0] = static_cast<uint8_t>(MI::CODE::ReportEnabledPlans);
    uint8_t *p = &txbuf[2];
    for (int i = 1; i <= groups.size(); i++)
    {
        Group *grp = GetGroup(i);
        for (int j = 1; j <= 255; j++)
        {
            if (grp->IsPlanEnabled(j))
            {
                *p++ = i;
                *p++ = j;
            }
        }
    }
    int bytes = p - txbuf;
    txbuf[1] = (bytes - 2) / 2;
    return bytes;
}
