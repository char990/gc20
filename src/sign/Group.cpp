#include <sign/Group.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>

using namespace Utils;

Group::Group(uint8_t groupId)
:groupId(groupId)
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

void Group::Add(Sign *sign)
{
    vSigns.push_back(sign);
    sign->Reset();
    signCnt++;
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
        FcltSwitch();
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
                ExtInput();
            }
        }
    }

    PeriodicHook();
}

void Group::FcltSwitch()
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

void Group::ExtInput()
{
    if(extInput.IsChanged())
    {
        uint8_t msg=0;
        if( (dsExt->dispType == DISP_STATUS::TYPE::EXT && msg <= dsExt->fmpid[0]) ||
            (dsExt->dispType != DISP_STATUS::TYPE::Ext) )
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

    return false;
}

bool Group::IsPlanActive(uint8_t id)
{

    return false;
}

bool Group::IsPlanEnabled(uint8_t id)
{
    return DbHelper::Instance().uciProcess.IsPlanEnabled(groupId, id);
}

void Group::EnablePlan(uint8_t id)
{
    if(id==0)
    {
        DisablePlan(0);
        return;
    }
    DbHelper::Instance().uciProcess.EnablePlan(groupId, id);
}

void Group::DisablePlan(uint8_t id)
{
    DbHelper::Instance().uciProcess.DisablePlan(groupId, id);
}

bool Group::IsMsgActive(uint8_t p)
{

    return false;
}

bool Group::IsFrmActive(uint8_t p)
{

    return false;
}

void Group::DispBackup()
{
    for(auto sign : vSigns)
    {
        sign->DispBackup();
    }
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

void DispBackup()
{
    for(auto sign : vSigns)
    {
        sign->DispBackup();
    }
}

void Group::DispFrm(uint8_t id)
{
    DispNext(DISP_STATUS::TYPE::FRM, id);
}

void Group::DispMsg(uint8_t id)
{
    DispNext(DISP_STATUS::TYPE::MSG, id);
}

void Group::DispAtomicFrm(uint8_t *id)
{
    dsNext->dispType = DISP_STATUS::TYPE::ATF;
    for(int i=0;i<signCnt;i++)
    {
        dsNext->fmpid[i]=0;
        uint8_t* p = id;
        for(int j=0;i<signCnt;j++)
        {
            if(*p==vSigns[i]->SignId())
            {
                dsNext->fmpid[i] = *(p+1);
                break;
            }
            p+=2;
        }
        id+=2;
    }
}

void Group::DispExtSw(uint8_t id)
{
    DispNext(DISP_STATUS::TYPE::Ext, id);
}

void Group::SetDimming(uint8_t dimming)
{
    for(auto sign : vSigns)
    {
        sign->SetDimming(dimming);
    }
}

void Group::SetPower(uint8_t v)
{
    if(v==0)
    {
        // set power off
        power = PWR_STATE::OFF;
        dsBak.Frm0();
        dsCurrent.Frm0();
        dsNext.Frm0();
        dsExt.N_A();
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
            pwrUpTmr.Setms(DbHelper::Instance.uciProd.PowerOnDelay()*1000);
            power = PWR_STATE::RISING;
        }
    }
}

void Group::SignSetPower(uint8_t v)
{
    for(auto sign : vSigns)
    {
        sign->SetPower(power);
    }
}

void Group::SetDevice(uint8_t endis)
{
    for(auto sign : vSigns)
    {
        sign->SetDevice(endis);
    }
}

