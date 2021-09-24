#include <cstring>
#include <sign/Group.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/ptcpp.h>

using namespace Utils;

Group::Group(uint8_t groupId)
:groupId(groupId), taskPlnLine(0), taskMsgLine(0), msgEnd(1)
{
    pwrUpTmr.Setms(0);
    dsBak = nullptr;
    dsCurrent = nullptr;
    dsNext = nullptr;
    dsExt = nullptr;
    signCnt = 0;
}

Group::~Group()
{
    if(dsBak != nullptr)
    {
        delete dsBak;
    }
    if(dsCurrent != nullptr)
    {
        delete dsCurrent;
    }
    if(dsNext != nullptr)
    {
        delete dsNext;
    }
    if(dsExt != nullptr)
    {
        delete dsExt;
    }
}

void Group::Init()
{
    dsBak = new DispStatus(signCnt);
    dsCurrent = new DispStatus(signCnt);
    dsNext = new DispStatus(signCnt);
    dsExt = new DispStatus(signCnt);
    DispFrm(0);
    memset(plnMin, 0, sizeof(plnMin));
    // load pln
}

void Group::PeriodicRun()
{
    // components PeriodicRun
    fcltSw.PeriodicRun();
    extInput.PeriodicRun();

    if(fcltSw.IsChanged())
    {
        FcltSwitchFunc();
        fcltSw.ClearChangeFlag();
    }
    else
    {
        if(FacilitySwitch::FS_STATE::OFF != fcltSw.Get())
        {
            if(power == PWR_STATE::RISING)
            {
                if(pwrUpTmr.IsExpired())
                {
                    power = PWR_STATE::ON;
                    SignSetPower(1);
                    // reset sign
                }
            }
            if( power == PWR_STATE::ON &&
                FacilitySwitch::FS_STATE::AUTO == fcltSw.Get() )
            {
                ExtInputFunc();
            }
        }
    }

    bool reload = false;
    if(IsDsNextEmergency())
    {
        msgEnd=1;
    }
    if(msgEnd)
    {
        reload = LoadDsNext();
    }

    TaskPln(&taskPlnLine);
    TaskMsg(&taskMsgLine);
    //TaskFrm(&taskFrmLine);


    PeriodicHook();
}

void Group::FcltSwitchFunc()
{
    FacilitySwitch::FS_STATE fs = fcltSw.Get();
    if(fs ==FacilitySwitch::FS_STATE::OFF)
    {
        // PowerOutput(0);
        SetPower(0);
        // reset signs
    }
    else
    {
        if(power == PWR_STATE::OFF)
        {
            //PowerOutput(1);
            SetPower(1);
        }
        if(fs == FacilitySwitch::FS_STATE::AUTO)
        {
            DispNext(DISP_STATUS::TYPE::FRM, 0);
        }
        else
        {
            uint8_t msg = (fs == FacilitySwitch::FS_STATE::MSG1) ? 1 : 2;
            DispNext(DISP_STATUS::TYPE::FSW, msg);
        }
    }
}

void Group::ExtInputFunc()
{
    if(extInput.IsChanged())
    {
        uint8_t msg=0;
        if( (dsExt->dispType == DISP_STATUS::TYPE::EXT && msg <= dsExt->fmpid[0]) ||
            (dsExt->dispType != DISP_STATUS::TYPE::EXT) )
        {
            DispNext(DISP_STATUS::TYPE::EXT, msg);
        }
        extInput.ClearChangeFlag();
    }
}

bool Group::IsSignInGroup(uint8_t id)
{
    for(auto s: vSigns)
    {
        if(s->SignId()==id)
        {
            return true;
        }
    }
    return false;
}

bool Group::IsEnPlanOverlap(uint8_t id)
{
    if(id==0) return false;
    return false;
}

bool Group::IsPlanActive(uint8_t id)
{
    if(id==0)
    {// check all plans

    }

    return false;
}

bool Group::IsPlanEnabled(uint8_t id)
{
    if(id==0)
    {// check all plans

    }
    return DbHelper::Instance().uciProcess.IsPlanEnabled(groupId, id);
}

APP::ERROR Group::EnablePlan(uint8_t id)
{
    if(id==0)
    {
        return DisablePlan(0);
    }
    if (IsPlanActive(id))
    {
        return APP::ERROR::FrmMsgPlnActive;
    }
    if (IsPlanEnabled(id))
    {
        return APP::ERROR::PlanEnabled;
    }
    if (IsEnPlanOverlap(id))
    {
        return APP::ERROR::OverlaysNotSupported;
    }
    DbHelper::Instance().uciProcess.EnablePlan(groupId, id);
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::DisablePlan(uint8_t id)
{
    if (IsPlanActive(id))
    {
        return APP::ERROR::FrmMsgPlnActive;
    }
    if (id!=0 && !IsPlanEnabled(id))
    {
        return APP::ERROR::PlanNotEnabled;
    }
    DbHelper::Instance().uciProcess.DisablePlan(groupId, id);
    return APP::ERROR::AppNoError;
}

bool Group::IsMsgActive(uint8_t p)
{

    return false;
}

bool Group::IsFrmActive(uint8_t p)
{

    return false;
}

void Group::DispNext(DISP_STATUS::TYPE type, uint8_t id)
{
    if(type==DISP_STATUS::TYPE::EXT)
    {
        dsExt->dispType = type;
        dsExt->fmpid[0] = id;
    }
    else
    {
        dsNext->dispType = type;
        dsNext->fmpid[0] = id;
    }
}

void Group::DispBackup()
{
    for(auto sign : vSigns)
    {
        //sign->DispBackup();
    }
}

APP::ERROR Group::DispFrm(uint8_t id)
{
    if(FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
    {
        DispNext(DISP_STATUS::TYPE::FRM, id);
        return APP::ERROR::AppNoError;
    }
    return APP::ERROR::FacilitySwitchOverride;
}

APP::ERROR Group::DispMsg(uint8_t id)
{
    if(FacilitySwitch::FS_STATE::AUTO == fcltSw.Get())
    {
        DispNext(DISP_STATUS::TYPE::MSG, id);
        return APP::ERROR::AppNoError;
    }
    return APP::ERROR::FacilitySwitchOverride;
}

APP::ERROR Group::DispAtomicFrm(uint8_t *cmd)
{
    if(FacilitySwitch::FS_STATE::AUTO != fcltSw.Get())
    {
        return APP::ERROR::FacilitySwitchOverride;
    }
    if(cmd[2]=signCnt)
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t * p = cmd+3;
    for(int i=0;i<signCnt;i++)
    {
        // check if there is a duplicate sign id
        uint8_t * p2=p+2;
        for(int j=i+1;j<signCnt;j++)
        {
            if(*p==*p2)
            {
                return APP::ERROR::SyntaxError;
            }
            p2+=2;
        }
        // sign is in this group
        if ( !IsSignInGroup(*p) )
        {
            return APP::ERROR::UndefinedDeviceNumber;
        }
        p++;
        // frm is defined or frm0
        if( (*p != 0) && DbHelper::Instance().uciFrm.GetFrm(*p)==nullptr)
        {
            return APP::ERROR::FrmMsgPlnUndefined;
        }
        p++;
    }
    dsNext->dispType = DISP_STATUS::TYPE::ATF;
    p=cmd+3;
    for(int i=0;i<signCnt;i++)
    {
        dsNext->fmpid[i]=0;
        for(int j=0;i<signCnt;j++)
        {
            if(*p==vSigns[i]->SignId())
            {// matched sign id
                dsNext->fmpid[i] = *(p+1);
                break;
            }
        }
        p+=2;
    }
    return APP::ERROR::AppNoError;
}

void Group::DispExtSw(uint8_t id)
{
    DispNext(DISP_STATUS::TYPE::EXT, id);
}

APP::ERROR Group::SetDimming(uint8_t dimming)
{
    for(auto sign : vSigns)
    {
        sign->DimmingSet(dimming);
    }
    return APP::ERROR::AppNoError;
}

APP::ERROR Group::SetPower(uint8_t v)
{
    if(v==0)
    {
        // set power off
        power = PWR_STATE::OFF;
        dsBak->Frm0();
        dsCurrent->Frm0();
        dsNext->Frm0();
        dsExt->N_A();
        for(auto sign : vSigns)
        {
            sign->Reset();
        }
    }
    else
    {
        // set power on
        if(power == PWR_STATE::OFF)
        {
            pwrUpTmr.Setms(DbHelper::Instance().uciProd.SlavePowerUpDelay()*1000);
            power = PWR_STATE::RISING;
        }
    }
    return APP::ERROR::AppNoError;
}

void Group::SignSetPower(uint8_t v)
{
    for(auto sign : vSigns)
    {
        //sign->SetPower(power);
    }
}

APP::ERROR Group::SetDevice(uint8_t endis)
{
    for(auto sign : vSigns)
    {
        //sign->SetDevice(endis);
    }
    return APP::ERROR::AppNoError;
}


bool Group::TaskPln(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        printf("TaskPln Step 0, delay 1 sec\n");
        task1Tmr.Setms(1000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
        printf("TaskPln Step 1, delay 3 sec\n");
        task1Tmr.Setms(3000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
        printf("TaskPln Step 2, delay 5 sec\n");
        task1Tmr.Setms(5000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());

        if(dsCurrent->dispType == DISP_STATUS::TYPE::PLN)
        {
            
        }
    };
    PT_END();
}

bool Group::TaskMsg(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        if(1)//new msg
        {
            if(1)// msg is defined
            {
                // is this msg same Ext-input
                // 
                // else
                // Load 1st frm
                // load ok?
                // display 1st frm
                // display ok?
                // if first frm is overlay, set/clear or-buf
                // if 2nd frm, load 2nd frm with or-buf
                // load ok?
            }
            else
            {
                // only update msg id
            }
        }
        else
        {
            // msg end? set endMsg
            // is this msg Ext-input
        }
        printf("TaskMsg, every 2 sec\n");
        task2Tmr.Setms(2000);
        PT_WAIT_UNTIL(task2Tmr.IsExpired());
    };
    PT_END();
}

bool Group::IsDsNextEmergency()
{
    bool r=false;
    if (dsExt->dispType == DISP_STATUS::TYPE::EXT)
    {// external input
        uint8_t mid = dsExt->fmpid[0];
        if(mid>=3 && mid<=5)
        {
            if(DbHelper::Instance().uciUser.ExtSwCfgX(mid)->emergency)
            {
                r=true;
            }
        }
    }
    return r;
}

bool Group::LoadDsNext()
{
    bool r=false;
    if (dsNext->dispType == DISP_STATUS::TYPE::FSW)
    {// facility switch
        dsBak->Frm0();
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r=true;
    }
    else if (dsExt->dispType == DISP_STATUS::TYPE::EXT)
    {// external input
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsExt);
        dsExt->N_A();
        r=true;
    }
    else if (dsNext->dispType != DISP_STATUS::TYPE::N_A)
    {// display command
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r=true;
    }
    if (dsCurrent->dispType == DISP_STATUS::TYPE::N_A)
    { // if current == N/A, load frm[0] to activate plan
        dsBak->Frm0();
        dsCurrent->Frm0();
        r=true;
    }
    if (dsCurrent->fmpid[0] == 0 &&
        (dsCurrent->dispType == DISP_STATUS::TYPE::FRM || dsCurrent->dispType == DISP_STATUS::TYPE::MSG))
    {//frm[0] or msg[0], activate plan
        dsCurrent->dispType = DISP_STATUS::TYPE::PLN;
        r=true;
    }
    return r;
}

/*
    uint8_t signId, power;
    uint8_t dimmingSet, dimmingV;
    uint8_t deviceSet, deviceV;

    DispStatus dsBak;
    DispStatus dsCurrent;
    DispStatus dsNext;
    DispStatus dsExt;
        uint8_t currentPln, currentMsg, currentFrm;
*/
