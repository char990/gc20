#include <fcntl.h>

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

using namespace Utils;
extern time_t GetTime(time_t *);

Controller::Controller()
    : db(DbHelper::Instance()),
      groups(DbHelper::Instance().GetUciProd().NumberOfGroups()),
      signs(DbHelper::Instance().GetUciProd().NumberOfSigns())
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

    UciProd &prod = db.GetUciProd();

    ctrllerError.SetV(db.GetUciProcess().CtrllerErr());
    overtempFault.SetCNT(prod.OverTempDebounce());
    overtempFault.SetState(ctrllerError.IsSet(DEV::ERROR::EquipmentOverTemperature));
    tempTmr.Setms(0);
    ms100Tmr.Setms(0);

    long dt = db.GetUciUserCfg().DisplayTimeoutMin();
    if (dt == 0)
    {
        displayTimeout.Clear();
    }
    else
    {
        displayTimeout.Setms(dt * 60000);
    }

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
        throw std::invalid_argument(FmtException("Unknown ProdType:%d", static_cast<int>(prod.ProdType())));
        break;
    }
    for (int i = 0; i < groups.size(); i++)
    {
        auto &gs = groups.at(i)->GetSigns();
        for (auto &s : gs)
        {
            signs[s->SignId() - 1] = s;
        }
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
        throw std::runtime_error("Re-invoke Controller::Init() is not allowed");
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
                Ldebug("ETH1 restart...");
                system("ifdown ETH1");
                system("ifup ETH1");
                Ldebug("Done.");
                tcpServer->Open();
            }
            if (rr_flag & RQST_REBOOT)
            {
                const char *_re = " -> -> -> reboot";
                evt.Push(0, _re);
                Ldebug("\n%s...", _re);
                system("reboot");
            }
            if (rr_flag & RQST_RESTART)
            {
                const char *_re = " -> -> -> restart";
                Ldebug("\n%s...", _re);
                system("sync");
                throw std::runtime_error(_re);
            }
            rr_flag = 0;
            rrTmr.Clear();
        }
    }

    if (displayTimeout.IsExpired())
    {
        if (!IsPlnActive(0))
        {
            ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 1);
            for (auto &g : groups)
            {
                g->DispFrm(0, true);
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

    if (ms100Tmr.IsExpired())
    { // 100ms
        ms100Tmr.Setms(100);
        ExtInputFunc();
        PowerMonitor();
    }

    if (tempTmr.IsExpired())
    { // every 1 second
        tempTmr.Setms(1000);
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
            auto ot = db.GetUciUserCfg().OverTemp();
            if (curTemp >= ot)
            {
                overtempFault.Check(1);
                if (!ctrllerError.IsSet(DEV::ERROR::EquipmentOverTemperature) && overtempFault.IsHigh())
                {
                    ctrllerError.Push(DEV::ERROR::EquipmentOverTemperature, true);
                    db.GetUciAlarm().Push(0, "Controller OverTemperatureAlarm ONSET: %d'C", curTemp);
                    overtempFault.ClearEdge();
                }
            }
            else
            {
                overtempFault.Check(0);
                if (ctrllerError.IsSet(DEV::ERROR::EquipmentOverTemperature) && overtempFault.IsLow())
                {
                    ctrllerError.Push(DEV::ERROR::EquipmentOverTemperature, false);
                    db.GetUciAlarm().Push(0, "Controller OverTemperatureAlarm CLEAR: %d'C", curTemp);
                    overtempFault.ClearEdge();
                }
            }
        }
    }

    for (auto &g : groups)
    {
        g->PeriodicRun();
    }
}

/*
void Controller::WriteFifo(char c)
{
    int fifo_fd = open("/tmp/goblin_fifo", O_WRONLY | O_NONBLOCK);
    if (fifo_fd != -1)
    {
        write(fifo_fd, &c, 1);
        close(fifo_fd);
    }
}
*/
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
    for (int i = 0; i < extInput.size(); i++)
    {
        auto gin = extInput.at(i);
        gin->PeriodicRun();
        if (gin->IsRising())
        {
            gin->ClearRising();
            uint8_t msg = i + 3;
            const char *fmt = "Leading edge of external input[%d]";
            db.GetUciEvent().Push(0, fmt, i + 1);
            Ldebug(fmt, i + 1);
            for (auto &g : groups)
            {
                g->DispExt(msg);
            }
            // only the highest priority will be triggered, ignore others
            while (++i < extInput.size())
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
    long ms = db.GetUciUserCfg().DisplayTimeoutMin();
    if (ms > 0)
    {
        displayTimeout.Setms(ms * 60000);
        ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 0);
    }
}

void Controller::RefreshSessionTime()
{
    long ms = db.GetUciUserCfg().SessionTimeoutSec();
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

APP::ERROR Controller::CmdSystemReset(uint8_t *cmd, char *rejectStr)
{
    auto grpId = cmd[1];
    auto lvl = cmd[2];
    if (lvl > 3 && lvl < 255)
    {
        snprintf(rejectStr, 63, "Invalid SystemReset level[%d]", lvl);
        return APP::ERROR::SyntaxError;
    }
    if (lvl > 1 && grpId != 0)
    {
        snprintf(rejectStr, 63, "Group[%d]SystemReset Level[%d] is only for Group[0])", grpId, lvl);
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (grpId > groups.size())
    {
        snprintf(rejectStr, 63, "Group[%d] undefined", grpId);
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
            if (db.GetUciProd().IsResetLogAllowed())
            {
                db.GetUciFault().Reset();
            }
            overtempFault.SetState(false);
            ctrllerError.Clear();
            db.GetUciProcess().SaveCtrllerErr(ctrllerError.GetV());
        }
        if (lvl >= 3)
        {
            if (db.GetUciProd().ProdType() == PRODUCT::VMS)
            { // Clearing all frame/msg is for VMS only
                // ISLUS's frame/msg can not be changed
                db.GetUciFrm().Reset();
                db.GetUciMsg().Reset();
            }
            db.GetUciPln().Reset();
        }
        if (lvl == 255)
        {
            db.GetUciUserCfg().LoadFactoryDefault();
        }
    }
    db.GetUciEvent().Push(0, "SystemReset: Group[%d]Level=%d", grpId, lvl);
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdDispFrm(uint8_t *cmd)
{
    auto grpId = cmd[1];
    auto frmId = cmd[2];
    auto grp = GetGroup(grpId);
    if (grp == nullptr)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (frmId != 0)
    {
        if (!db.GetUciFrm().IsFrmDefined(frmId))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        auto &prod = db.GetUciProd();
        if (prod.ProdType() == PRODUCT::ISLUS)
        {
            std::vector<uint8_t> frms(prod.NumberOfSigns(), 0);
            auto &gs = grp->GetSigns();
            for (auto &s : gs)
            {
                frms.at(s->SignId() - 1) = frmId;
            }
            if (CheckAdjacentLane(frms) != APP::ERROR::AppNoError)
            {
                return APP::ERROR::OverlaysNotSupported;
            }
        }
    }
    return grp->DispFrm(frmId, true);
}

APP::ERROR Controller::CmdSignTest(uint8_t *cmd)
{
    auto grpId = cmd[2];
    auto colourId = cmd[3];
    auto pixels = cmd[4];
    auto grp = GetGroup(grpId);
    if (grp == nullptr)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (pixels > 5)
    {
        return APP::ERROR::FrmMsgPlnUndefined;
    }
    if (colourId > 9)
    {
        return APP::ERROR::ColourNotSupported;
    }
    auto &prod = DbHelper::Instance().GetUciProd();
    auto rows = prod.PixelRows();
    auto columns = prod.PixelColumns();
    auto corelen = prod.Gfx1CoreLen();
    int f_offset;
    std::vector<uint8_t> frm;
    if (rows < 255 && columns < 255)
    {
        f_offset = 9;
        frm.resize(corelen + f_offset + 2);
        frm[0] = static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame);
        frm[1] = 255;
        frm[2] = 255;
        frm[3] = rows;
        frm[4] = columns;
        frm[5] = colourId;
        frm[6] = 0;
        Utils::Cnvt::PutU16(corelen, frm.data() + 7);
    }
    else
    {
        f_offset = 13;
        frm.resize(corelen + f_offset + 2);
        frm[0] = static_cast<uint8_t>(MI::CODE::SignSetHighResolutionGraphicsFrame);
        frm[1] = 255;
        frm[2] = 255;
        Cnvt::PutU16(rows, frm.data() + 3);
        Cnvt::PutU16(columns, frm.data() + 5);
        frm[7] = colourId;
        frm[8] = 0;
        Utils::Cnvt::PutU32(corelen, frm.data() + 9);
    }

    if (pixels == 1 || pixels == 2)
    { // odd(1)/even(2) rows
        uint8_t *core = frm.data() + f_offset;
        memset(core, 0, corelen);
        int bitOffset = 0;
        for (int j = pixels - 1; j < rows; j += 2)
        {
            for (int i = 0; i < columns; i++)
            {
                BitOffset::Set70Bit(core, bitOffset++);
            }
            bitOffset += columns;
        }
    }
    else
    {
        memset(frm.data() + f_offset, (pixels == 0) ? 0xFF : ((pixels == 3) ? 0x55 : 0xAA), corelen);
    }

    Cnvt::PutU16(Crc::Crc16_1021(frm.data(), frm.size() - 2), frm.data() + frm.size() - 2);
    UciFrm &ucifrm = db.GetUciFrm();
    auto r = ucifrm.SetFrm(frm.data(), frm.size());
    if (r != APP::ERROR::AppNoError)
    {
        return r;
    }
    ucifrm.SaveFrm(255);
    db.GetUciEvent().Push(0, "SignTest:SetFrame[255]:Colour:%s,Pixels:%s",
                          FrameColour::COLOUR_NAME[colourId], TestPixels[pixels]);
    r = grp->DispFrm(255, true);
    if (r == APP::ERROR::AppNoError)
    {
        db.GetUciEvent().Push(0, "SignTest:Group[%d]DispFrm[255]", grpId);
    }
    return r;
}

APP::ERROR Controller::CmdDispMsg(uint8_t *cmd)
{
    auto grp = GetGroup(cmd[1]);
    if (grp == nullptr)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    uint8_t msgId = cmd[2];
    if (msgId != 0)
    {
        if (!db.GetUciMsg().IsMsgDefined(msgId))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
    }
    return grp->DispMsg(msgId, true);
}

APP::ERROR Controller::CmdDispAtomicFrm(uint8_t *cmd, int len)
{
    auto grp = GetGroup(cmd[1]);
    if (grp == nullptr)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    auto gsCnt = cmd[2];
    if (gsCnt != grp->SignCnt())
    {
        return APP::ERROR::SyntaxError;
    }
    auto &prod = db.GetUciProd();
    if (prod.ProdType() == PRODUCT::ISLUS)
    {
        std::vector<uint8_t> frms(prod.NumberOfSigns(), 0);
        auto pnext = cmd + 3;
        for (int i = 0; i < gsCnt; i++)
        {
            auto sign_id = *pnext++;
            auto frm_id = *pnext++;
            // sign is in this group
            if (!grp->IsSignInGroup(sign_id))
            {
                return APP::ERROR::UndefinedDeviceNumber;
            }
            // frm is defined
            if (!db.GetUciFrm().IsFrmDefined(frm_id))
            {
                return APP::ERROR::FrmMsgPlnUndefined;
            }
            frms.at(sign_id - 1) = frm_id;
        }
        if (CheckAdjacentLane(frms) != APP::ERROR::AppNoError)
        {
            return APP::ERROR::OverlaysNotSupported;
        }
    }
    return grp->DispAtmFrm(cmd, true);
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
    else if (plnId != 0 && !db.GetUciPln().IsPlnDefined(plnId))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else
    {
        r = GetGroup(grpId)->EnDisPlan(plnId, cmd[0] == static_cast<uint8_t>(MI::CODE::EnablePlan));
    }
    return r;
}

APP::ERROR Controller::CmdSetDimmingLevel(uint8_t *cmd, char *rejectStr)
{
    uint8_t entry = cmd[1];
    if (entry == 0 || entry > groups.size())
    {
        snprintf(rejectStr, 63, "Invalid number of entries:%d", entry);
        return (APP::ERROR::SyntaxError);
    }
    uint8_t *p;
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] > groups.size())
        {
            snprintf(rejectStr, 63, "Invalid group id:%d", p[0]);
            return APP::ERROR::UndefinedDeviceNumber;
        }
        if (p[1] != 0 && (p[2] == 0 || p[2] > 16))
        {
            snprintf(rejectStr, 63, "Dimming Level Not Supported:%d", p[2]);
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
    char buf[64];
    int len = sprintf(buf, "SetDimming:");
    p = cmd + 2;
    for (int i = 0; i < cmd[1]; i++)
    {
        len += snprintf(buf + len, 63 - len, " Grp%d(%d-%d)", p[0], p[1], p[2]);
        p += 3;
    }
    db.GetUciEvent().Push(0, buf);
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdPowerOnOff(uint8_t *cmd, char *rejectStr)
{
    uint8_t entry = cmd[1];
    if (entry == 0 || entry > groups.size())
    {
        snprintf(rejectStr, 63, "Invalid number of entries:%d", entry);
        return (APP::ERROR::SyntaxError);
    }
    uint8_t *p;
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] > groups.size())
        {
            snprintf(rejectStr, 63, "Invalid group id:%d", p[0]);
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
    char buf[64];
    int slen = sprintf(buf, "Power ON/OFF:");
    p = cmd + 2;
    for (int i = 0; i < cmd[1] && slen < 63; i++)
    {
        slen += snprintf(buf + slen, 63 - slen, " Grp%d(%d)", p[0], p[1]);
        p += 2;
    }
    db.GetUciEvent().Push(0, buf);
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdDisableEnableDevice(uint8_t *cmd, char *rejectStr)
{
    uint8_t entry = cmd[1];
    if (entry == 0 || entry > groups.size())
    {
        snprintf(rejectStr, 63, "Invalid number of entries:%d", entry);
        return (APP::ERROR::SyntaxError);
    }
    uint8_t *p;
    p = cmd + 2;
    for (int i = 0; i < entry; i++)
    {
        if (p[0] > groups.size())
        {
            snprintf(rejectStr, 63, "Invalid group id:%d", p[0]);
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
    char buf[64];
    int slen = sprintf(buf, "Dis/EnableDevice:");
    p = cmd + 2;
    for (int i = 0; i < cmd[1] && slen < 63; i++)
    {
        slen += snprintf(buf + slen, 63 - slen, " Grp%d(%d)", p[0], p[1]);
        p += 2;
    }
    db.GetUciEvent().Push(0, buf);
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

APP::ERROR Controller::CheckAdjacentLane(std::vector<uint8_t> &frms)
{
// ISLUS lane checking
#define UP_LEFT_0 182
#define UP_RIGHT_0 183
#define DN_LEFT_0 184
#define DN_RIGHT_0 185
#define RED_CROSS_0 189

#define UP_LEFT_1 192
#define UP_RIGHT_1 193
#define DN_LEFT_1 194
#define DN_RIGHT_1 195
#define RED_CROSS_1 199

#define IS_LEFT(frm) (frm == DN_LEFT_0 || frm == DN_LEFT_1)
#define IS_RIGHT(frm) (frm == DN_RIGHT_0 || frm == DN_RIGHT_1)
#define IS_CROSS(frm) (frm == RED_CROSS_0 || frm == RED_CROSS_1)

    auto signCnt = frms.size();
    for (int i = 0; i < signCnt; i++)
    {
        auto frm_id = frms.at(i);
        if (frm_id == 0)
        {
            frms.at(i) = frm_id = signs.at(i)->ReportFrm();
        }
        // check lane merge
        if (i > 0)
        {
            auto frm_left = frms.at(i - 1);
            if ((IS_RIGHT(frm_left) && (IS_CROSS(frm_id) || IS_LEFT(frm_id))) ||
                (IS_CROSS(frm_left) && IS_LEFT(frm_id)))
            { // merge to closed lane
                return APP::ERROR::OverlaysNotSupported;
            }
        }
    }
    return APP::ERROR::AppNoError;
#undef IS_CROSS
#undef IS_RIGHT
#undef IS_LEFT
}

APP::ERROR Controller::CmdUpdateTime(struct tm &stm)
{
    if (Time::IsTmValid(stm))
    {
        time_t t = mktime(&stm);
        if (t > 0)
        {
            char buf[64];
            char *p = buf + sprintf(buf, "UpdateTime:");
            p = Time::ParseTimeToLocalStr(GetTime(nullptr), p);
            sprintf(p, "->");
            Time::ParseTimeToLocalStr(t, p + 2);
            db.GetUciEvent().Push(0, buf);
            Ldebug("%s", buf);
            if (Time::SetLocalTime(stm) < 0)
            {
                const char *s = "UpdateTime: Set system time failed(MemoryError)";
                Ldebug(s);
                db.GetUciAlarm().Push(0, s);
                db.GetUciFault().Push(0, DEV::ERROR::MemoryError, 1);
            }
            else
            {
                if (pDS3231->SetTimet(t) < 0)
                {
                    const char *s = "UpdateTime: Set DS3231 time failed(MemoryError)";
                    Ldebug(s);
                    db.GetUciAlarm().Push(0, s);
                    db.GetUciFault().Push(0, DEV::ERROR::MemoryError, 1);
                }
            }
            return APP::ERROR::AppNoError;
        }
    }
    return APP::ERROR::SyntaxError;
}

APP::ERROR Controller::SignSetFrame(uint8_t *data, int len, char *rejectStr)
{
    auto r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_FRM_ID);
    if (id == 0)
    {
        snprintf(rejectStr, 63, "Frame[0] is not valid");
        r = APP::ERROR::SyntaxError;
    }
    else if (IsFrmActive(id))
    {
        snprintf(rejectStr, 63, "Frame[%d] is active", id);
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else if (id <= db.GetUciUserCfg().LockedFrm()) // && pstatus->rFSstate() != Status::FS_OFF)
    {
        snprintf(rejectStr, 63, "Frame[%d] is locked", id);
        r = APP::ERROR::OverlaysNotSupported; // locked frame
    }
    else if (db.GetUciProd().ProdType() == PRODUCT::ISLUS && db.GetUciProd().IsIslusSpFrm(id))
    {
        snprintf(rejectStr, 63, "Frame[%d] is special ISLUS frame, can't be changed", id);
        r = APP::ERROR::OverlaysNotSupported; // pre-defined ISLUS special framaes and can't be changed
    }
    else
    {
        UciFrm &frm = db.GetUciFrm();
        r = frm.SetFrm(data, len);
        if (r == APP::ERROR::AppNoError)
        {
            frm.SaveFrm(id);
            switch (*data)
            {
            case static_cast<uint8_t>(MI::CODE::SignSetTextFrame):
                db.GetUciEvent().Push(0, "SetTxtFrame: Frame[%d]", id);
                break;
            case static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame):
                db.GetUciEvent().Push(0, "SetGfxFrame: Frame[%d]", id);
                break;
            default:
                db.GetUciEvent().Push(0, "SetHiResGfxFrame: Frame[%d]", id);
                break;
            }
        }
    }
    return r;
}
