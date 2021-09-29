#include <sign/Scheduler.h>
#include <sign/GroupVms.h>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>

Scheduler::Scheduler()
{
}

Scheduler::~Scheduler()
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

void Scheduler::Init(TimerEvent *tmrEvt_)
{
    if (tmrEvt != nullptr)
    {
        MyThrow("Re-invoke Scheduler::Init()");
    }
    tmrEvt = tmrEvt_;
    tmrEvt->Add(this);
    displayTimeout.Clear();
    UciProd & prod = DbHelper::Instance().GetUciProd(); 
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

void Scheduler::PeriodicRun()
{ // run every 10ms
    if (displayTimeout.IsExpired())
    {
        // log
        displayTimeout.Clear();
        for (int i = 0; i < groupCnt; i++)
        {
            groups[i]->DispFrm(0);
        }
    }

    for (int i = 0; i < groupCnt; i++)
    {
        groups[i]->PeriodicRun();
    }
}

void Scheduler::RefreshDispTime()
{
    long ms = DbHelper::Instance().GetUciUser().DisplayTimeout();
    ms = (ms == 0) ? -1 : (ms * 1000);
    displayTimeout.Setms(ms);
}

void Scheduler::SessionLed(uint8_t v)
{
    sessionLed = v;
}

uint8_t Scheduler::CtrllerErr()
{
    return ctrllerErr;
}

bool Scheduler::IsFrmActive(uint8_t id)
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

bool Scheduler::IsMsgActive(uint8_t id)
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

bool Scheduler::IsPlnActive(uint8_t id)
{
    if (id > 0)
    {
        for (int i = 0; i < groupCnt; i++)
        {
            if (groups[i]->IsPlanActive(id))
            {
                return true;
            }
        }
    }
    return false;
}

APP::ERROR Scheduler::CmdDispFrm(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t frmId = cmd[2];
    if (grpId==0 || grpId > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    StFrm *frm;
    if (frmId != 0)
    {
        if (!DbHelper::Instance().GetUciFrm().IsFrmDefined(frmId))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
    }
    return GetGroup(grpId)->DispFrm(frmId);
}

APP::ERROR Scheduler::CmdDispMsg(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t msgId = cmd[2];
    if (grpId==0 || grpId > Scheduler::Instance().GroupCnt())
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

APP::ERROR Scheduler::CmdDispAtomicFrm(uint8_t *cmd, int len)
{
    uint8_t grpId = cmd[1];
    if (grpId == 0 || grpId > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    return GetGroup(grpId)->DispAtomicFrm(cmd);
}

APP::ERROR Scheduler::CmdEnablePlan(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t plnId = cmd[2];
    APP::ERROR r = APP::ERROR::AppNoError;
    if (grpId > groupCnt || grpId == 0)
    {
        r = APP::ERROR::UndefinedDeviceNumber;
    }
    else if (!DbHelper::Instance().GetUciPln().IsPlnDefined(plnId))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else
    {
        r = GetGroup(grpId)->EnablePlan(plnId);
    }
    return r;
}

APP::ERROR Scheduler::CmdDisablePlan(uint8_t *cmd)
{
    uint8_t grpId = cmd[1];
    uint8_t plnId = cmd[2];
    APP::ERROR r = APP::ERROR::AppNoError;
    if (grpId > groupCnt || grpId == 0)
    {
        r = APP::ERROR::UndefinedDeviceNumber;
    }
    else if (!DbHelper::Instance().GetUciPln().IsPlnDefined(plnId))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else
    {
        r = GetGroup(grpId)->DisablePlan(plnId);
    }
    return r;
}

APP::ERROR Scheduler::CmdSetDimmingLevel(uint8_t *cmd)
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

APP::ERROR Scheduler::CmdPowerOnOff(uint8_t *cmd, int len)
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

APP::ERROR Scheduler::CmdDisableEnableDevice(uint8_t *cmd, int len)
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

int Scheduler::CmdRequestEnabledPlans(uint8_t *txbuf)
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
