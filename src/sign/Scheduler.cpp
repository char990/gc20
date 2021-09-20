#include <sign/Scheduler.h>
#include <sign/Vms.h>
#include <sign/Islus.h>
#include <uci/DbHelper.h>
//#include <module/ptcpp.h>

Scheduler::Scheduler()
    : unitedSigns(nullptr), groups(nullptr), tmrEvt(nullptr)
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
            delete unitedSigns[i];
        }
        delete[] unitedSigns;
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
    unitedSigns = new UnitedSign *[signCnt];
    switch (DbHelper::Instance().uciProd.ProdType())
    {
    case 0:
        for (int i = 0; i < signCnt; i++)
        {
            unitedSigns[i] = new Vms(i + 1);
        }
        break;
    case 1:
        for (int i = 0; i < signCnt; i++)
        {
            unitedSigns[i] = new Islus(i + 1);
        }
        break;
    }
    AssignGroup();
}

void Scheduler::AssignGroup()
{
    if (groups != nullptr)
    {
        for (int i = 0; i < groupCnt; i++)
        {
            delete groups[i];
        }
        delete[] groups;
    }
    // group init
    groupCnt = DbHelper::Instance().uciUser.GroupCnt();
    groups = new Group *[groupCnt];
    for (int i = 0; i < groupCnt; i++)
    {
        groups[i] = new Group(i + 1);
    }
    for (int i = 1; i <= signCnt; i++)
    {
        GetGroup(DbHelper::Instance().uciUser.GetGroupIdOfSign(i))->Add(GetUnitedSign(i));
    }
    for (int i = 0; i < groupCnt; i++)
    {
        if (groups[i]->gUntSigns.size() == 0)
        {
            MyThrow("Error:There is no sign in Group[%d]", i + 1);
        }
    }
}

void Scheduler::PeriodicRun()
{ // run every 10ms
    if (displayTimeout.IsExpired())
    {
        displayTimeout.Clear();
        // if there is a plan, load plan
        // or blank
    }









    for (int i = 0; i < groupCnt; i++)
    {
        groups[i]->PeriodicRun();
    }
}

void Scheduler::RefreshDispTime()
{
    displayTimeout.Setms(100000);
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
    for(int i=1;i<=groupCnt;i++)
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
    for(int i=1;i<=groupCnt;i++)
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
    return APP::ERROR::MiNotSupported;

    if (cmd[1]==0 || cmd[1] > groupCnt)
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    Group *grp = GetGroup(i)
    if(cmd[2]!=grp->gUntSigns.Size())
    {
        return APP::ERROR::UndefinedDeviceNumber;
    }
    #if 0
    for(int i=0;i<cmd[2];i++)
    {
        if()
    }

    if (cmd[2] != 0)
    {
        if (!DbHelper::Instance().uciFrm.IsFrmDefined(cmd[2]))
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
    }
    if (cmd[1] == 0)
    {
        for(int i=1;i<=groupCnt;i++)
        {
            GetGroup(i)->dsNext.dispType = DISP_STATUS::TYPE::FRM;
            for(int j=0;j<MAX_SIGN_IN_GROUP;j++)
            {
                GetGroup(i)->dsNext.sid = 0;
                GetGroup(i)->dsNext.fmpid = cmd[2];
            }
        }
    }
    else
    {
        GetGroup(cmd[1])->dsNext.dispType = DISP_STATUS::TYPE::FRM;
        for(int j=0;j<MAX_SIGN_IN_GROUP;j++)
        {
            GetGroup(i)->dsNext.sid = 0;
            GetGroup(i)->dsNext.fmpid = cmd[2];
        }
    }
    #endif
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

APP::ERROR Scheduler::CmdSetDimmingLevel(uint8_t *cmd)
{
}

APP::ERROR Scheduler::CmdPowerOnOff(uint8_t *cmd, int len)
{
}

APP::ERROR Scheduler::CmdDisableEnableDevice(uint8_t *cmd, int len)
{
}
