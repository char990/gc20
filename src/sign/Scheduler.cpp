#include <sign/Scheduler.h>
#include <sign/GroupVms.h>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
//#include <module/ptcpp.h>

Scheduler::Scheduler()
    : signs(nullptr), groups(nullptr), tmrEvt(nullptr)
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
    if (unitedSigns != nullptr)
    {
        for (int i = 0; i < signCnt; i++)
        {
            delete signs[i];
        }
        delete[] signs;
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

    // sign init
    signCnt = DbHelper::Instance().uciProd.NumberOfSigns();
    signs = new Sign *[signCnt];
    switch (DbHelper::Instance().uciProd.ExtStsRplSignType())
    {
    case SESR::SIGN_TYPE::TEXT:
        for (int i = 0; i < signCnt; i++)
        {
            signs[i] = new SignTxt(i + 1);
        }
        break;
    case SESR::SIGN_TYPE::GFX:
        for (int i = 0; i < signCnt; i++)
        {
            signs[i] = new SignGfx(i + 1);
        }
        break;
    case SESR::SIGN_TYPE::ADVGFX:
        for (int i = 0; i < signCnt; i++)
        {
            signs[i] = new SignAdg(i + 1);
        }
        break;
    }
    groupCnt = DbHelper::Instance().uciUser.GroupCnt();
    groups = new Group *[groupCnt];

    switch (DbHelper::Instance().uciProd.ProdType())
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
    for (int i = 1; i <= signCnt; i++)
    {
        GetGroup(DbHelper::Instance().uciUser.GetGroupIdOfSign(i))->Add(GetSign(i));
    }
    for (int i = 0; i < groupCnt; i++)
    {
        if (groups[i]->vSigns.size() == 0)
        {
            MyThrow("Error:There is no sign in Group[%d]", i + 1);
        }
        groups[i].Init();
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
    long ms = DbHelper::Instance().uciUser.DisplayTimeout();
    ms = (ms == 0) ? LONG_MAX : (ms * 1000);
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
    if (cmd[1] > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    StFrm *frm;
    if (cmd[2] != 0)
    {
        if (!DbHelper::Instance().uciFrm.IsFrmDefined(cmd[2]))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
    }
    for (int i = 1; i <= groupCnt; i++)
    {
        Group *grp = GetGroup(i);
        if (cmd[1] == 0 || cmd[1] == grp->GroupId())
        {
            grp->DispFrm(cmd[2]);
        }
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Scheduler::CmdDispMsg(uint8_t *cmd)
{
    if (cmd[1] > Scheduler::Instance().GroupCnt())
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    Message *msg;
    if (cmd[2] != 0)
    {
        if (!DbHelper::Instance().uciMsg.IsMsgDefined(cmd[2]))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
    }
    for (int i = 1; i <= groupCnt; i++)
    {
        Group *grp = GetGroup(i);
        if (cmd[1] == 0 || cmd[1] == grp->GroupId())
        {
            grp->DispMsg(cmd[2]);
        }
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Scheduler::CmdDispAtomicFrm(uint8_t *cmd, int len)
{
    if (cmd[1] == 0 || cmd[1] > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    Group *grp = GetGroup(i);
    if(grp->SignCnt()!=cmd[2])
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t * p = cmd+3;
    for(int i=0;i<cmd[2];i++)
    {
        uint8_t * p2=p+2;
        for(int j=i+1;j<cmd[2];j++)
        {
            if(*p==*p2)
            {
                return APP::ERROR::SyntaxError;
            }
            p2+=2;
        }
        if ( ! grp->IsSignInGroup(*p) )
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        p++;
        if( (*p != 0) && DbHelper::Instance().uciFrm.GetFrm(*p)==nullptr)
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        p++;
    }
    grp->DispAtomicFrm(cmd+3);
    return APP::ERROR::AppNoError;
}

APP::ERROR Scheduler::CmdEnablePlan(uint8_t *cmd)
{
    APP::ERROR r = APP::ERROR::AppNoError;
    if (cmd[1] > groupCnt || cmd[1] == 0)
    {
        r = APP::ERROR::UndefinedDeviceNumber;
    }
    else if (!DbHelper::Instance().uciPln.IsPlnDefined(cmd[2]))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (0) //Status::FS_AUTO != pstatus->rFSstate())
    {
        r = APP::ERROR::FacilitySwitchOverride; // 0x10  facility switch override
    }
    else
    {
        Group *grp = GetGroup(cmd[1]);
        uint8_t id = cmd[2];
        if (id == 0)
        {
            if (grp->IsPlanActive(id))
            {
                r = APP::ERROR::FrmMsgPlnActive;
            }
        }
        else
        {
            if (grp->IsPlanEnabled(id))
            {
                r = APP::ERROR::PlanEnabled;
            }
            else
            {
                if (grp->IsEnPlanOverlap(id))
                {
                    r = APP::ERROR::OverlaysNotSupported;
                }
            }
        }
        if (r == APP::ERROR::AppNoError)
        {
            grp->EnablePlan(id);
        }
    }
    return r;
}

APP::ERROR Scheduler::CmdDisablePlan(uint8_t *cmd)
{
    APP::ERROR r = APP::ERROR::AppNoError;
    if (cmd[1] > groupCnt || cmd[1] == 0)
    {
        r = APP::ERROR::UndefinedDeviceNumber;
    }
    else if (!DbHelper::Instance().uciPln.IsPlnDefined(cmd[2]))
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else
    {
        Group *grp = GetGroup(cmd[1]);
        uint8_t id = cmd[2];
        if (id == 0)
        {
            if (grp->IsPlanActive(id))
            {
                r = APP::ERROR::FrmMsgPlnActive;
            }
        }
        else
        {
            if (!grp->IsPlanEnabled(id))
            {
                r = APP::ERROR::PlanNotEnabled;
            }
        }
        if (r == APP::ERROR::AppNoError)
        {
            grp->DisablePlan(id);
        }
    }
    return r;
}

APP::ERROR Scheduler::CmdSetDimmingLevel(uint8_t *data)
{
    uint8_t entry = data[1];
    uint8_t *p;
    p = data + 2;
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
    p = data + 2;
    for (int i = 0; i < entry; i++)
    {
        uint8_t dimming = (p[1] == 0) ? 0 : p[2];
        if (p[0] == 0)
        {
            for (int i = 0; i < gcnt; i++)
            {
                groups[i]->SetDimming(dimming);
            }
        }
        else
        {
            groups[p[0] - 1]->SetDimming(dimming);
        }
        p += 3;
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Scheduler::CmdPowerOnOff(uint8_t *cmd, int len)
{
}

APP::ERROR Scheduler::CmdDisableEnableDevice(uint8_t *cmd, int len)
{
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