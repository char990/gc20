#include <sign/Controller.h>
#include <sign/GroupVms.h>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>
#include <module/MyDbg.h>

Controller::Controller()
{
}

Controller::~Controller()
{
    if (groups != nullptr)
    {
        for (int i = 0; i < groupCnt; i++)
        {
            delete groups[i];
        }
        delete[] groups;
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

    ctrllerError.SetV(DbHelper::Instance().GetUciProcess().CtrllerErr().Get());
    long ms = DbHelper::Instance().GetUciUser().DisplayTimeout();
    if(ms == 0)
    {
        displayTimeout.Clear();
        ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 0);
    }
    else
    {
        displayTimeout.Setms(ms * 1000);
    }

    UciProd &prod = DbHelper::Instance().GetUciProd();
    groupCnt = prod.NumberOfGroups();
    groups = new Group *[groupCnt];
    switch (prod.ProdType())
    {
    case 0: // vms
        for (int i = 0; i < groupCnt; i++)
        {
            groups[i] = new GroupVms(i + 1);
        }
        break;
    case 1: // islus
        for (int i = 0; i < groupCnt; i++)
        {
            groups[i] = new GroupIslus(i + 1);
        }
        break;
    }
}

void Controller::PeriodicRun()
{ // run every 10ms
    if (displayTimeout.IsExpired())
    {
        if(!IsPlnActive(0))
        {
            PrintDbg("displayTimeout!!!\n");
            ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 1);
            for (int i = 0; i < groupCnt; i++)
            {
                groups[i]->DispFrm(0);
            }
        }
        displayTimeout.Clear();
    }

    for (int i = 0; i < groupCnt; i++)
    {
        groups[i]->PeriodicRun();
    }
}

void Controller::RefreshDispTime()
{
    PrintDbg("RefreshDispTime\n");
    long ms = DbHelper::Instance().GetUciUser().DisplayTimeout();
    (ms == 0) ? displayTimeout.Clear() : displayTimeout.Setms(ms * 1000);
    ctrllerError.Push(DEV::ERROR::DisplayTimeoutError, 0);
}

void Controller::SessionLed(uint8_t v)
{
    sessionLed = v;
}

bool Controller::IsFrmActive(uint8_t id)
{
    if (id > 0)
    {
        for (int i = 0; i < groupCnt; i++)
        {
            if (groups[i]->IsFrmActive(id))
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
        for (int i = 0; i < groupCnt; i++)
        {
            if (groups[i]->IsMsgActive(id))
            {
                return true;
            }
        }
    }
    return false;
}

bool Controller::IsPlnActive(uint8_t id)
{
    for (int i = 0; i < groupCnt; i++)
    {
        if (groups[i]->IsPlanActive(id))
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
    if (grpId > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (grpId != 0)
    {
        GetGroup(grpId)->SystemReset(lvl);
    }
    else
    {
        for (int i = 1; i <= groupCnt; i++)
        {
            GetGroup(i)->SystemReset(lvl);
        }
        auto &db = DbHelper::Instance();
        if (lvl >= 2)
        {
            db.GetUciFault().Reset();
        }
        if (lvl >= 3)
        {
            db.GetUciFrm().Reset();
            db.GetUciMsg().Reset();
            db.GetUciPln().Reset();
        }
        if (lvl == 255)
        {
        }
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Controller::CmdDispFrm(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t frmId = cmd[2];
    if (grpId == 0 || grpId > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    if (frmId != 0)
    {
        if (!DbHelper::Instance().GetUciFrm().IsFrmDefined(frmId))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
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
    if (grpId == 0 || grpId > groupCnt)
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
    if (grpId > groupCnt || grpId == 0)
    {
        r = APP::ERROR::UndefinedDeviceNumber;
    }
    else if (plnId != 0 && !DbHelper::Instance().GetUciPln().IsPlnDefined(plnId))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else
    {
        r = GetGroup(grpId)->EnDisPlan(plnId, cmd[0] == MI::CODE::EnablePlan);
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
        if (p[0] > groupCnt)
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
            for (int i = 1; i <= groupCnt; i++)
            {
                GetGroup(i)->SetDimming(dimming);
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
        if (p[0] > groupCnt)
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        p += 2;
    }
    p = cmd + 2;

    for (int i = 0; i < entry; i++)
    {
        uint8_t d = (p[1] == 0);
        if (p[0] == 0)
        {
            for (int i = 1; i <= groupCnt; i++)
            {
                GetGroup(i)->SetPower(d);
            }
        }
        else
        {
            GetGroup(p[0])->SetPower(d);
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
        if (p[0] > groupCnt)
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        p += 2;
    }
    p = cmd + 2;

    for (int i = 0; i < entry; i++)
    {
        uint8_t d = (p[1] == 0) ? 0 : 1;
        if (p[0] == 0)
        {
            for (int i = 1; i <= groupCnt; i++)
            {
                GetGroup(i)->SetDevice(d);
            }
        }
        else
        {
            GetGroup(p[0])->SetDevice(d);
        }
        p += 2;
    }
    return APP::ERROR::AppNoError;
}

int Controller::CmdRequestEnabledPlans(uint8_t *txbuf)
{
    txbuf[0] = MI::CODE::ReportEnabledPlans;
    uint8_t *p = &txbuf[2];
    for (int i = 1; i <= groupCnt; i++)
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
