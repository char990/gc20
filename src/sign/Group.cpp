#include <sign/Group.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>

using namespace Utils;

Group::Group(uint8_t groupId)
:groupId(groupId)
{
    pwrUpTmr.Setms(0);
}

Group::~Group()
{

}

void Group::Add(UnitedSign *sign)
{
    gUntSigns.push_back(sign);
    sign->Reset();
}

void Group::PeriodicRun()
{
    fcltSw.PeriodicRun();
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

    for(auto s: gUntSigns)
    {
        s->PeriodicRun();
    }
}

void Group::FcltSwitch()
{
    FacilitySwitch::FS_STATE fs = fcltSw.Get();
    if(fs ==FacilitySwitch::FS_STATE::OFF)
    {
        // PowerOutput(0);
        power = PWR_STATE::OFF;
        SetPower(0);
        // reset signs
    }
    else
    {
        if(power == PWR_STATE::OFF)
        {
            // PowerOutput(1);
            pwrUpTmr.Setms(DbHelper::Instance.uciProd.PowerOnDelay());
        }
        elseif(fs == FacilitySwitch::FS_STATE::AUTO)
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

}

bool Group::IsSignInGroup(uint8_t id)
{
    for(auto s: gUntSigns)
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
    return DbHelper::Instance().uciGrpPln.IsPlanEnabled(groupId, id);
}

void Group::EnablePlan(uint8_t id)
{
    if(id==0)
    {
        DisablePlan(0);
        return;
    }
    DbHelper::Instance().uciGrpPln.EnablePlan(groupId, id);
}

void Group::DisablePlan(uint8_t id)
{
    DbHelper::Instance().uciGrpPln.DisablePlan(groupId, id);
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
    for(auto sign : gUntSigns)
    {
        sign->DispBackup();
    }
}

void Group::DispNext(DISP_STATUS::TYPE type,uint8_t id)
{
    for(auto sign : gUntSigns)
    {
        sign->DispNext(DISP_STATUS::TYPE type,uint8_t id);
    }
}

void DispBackup()
{
    for(auto sign : gUntSigns)
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

void Group::DispAtomicFrm(uint8_t *cmd, int len)
{
    for(auto sign : gUntSigns)
    {
        uint8_t* id = cmd[3];
        for(int i=0;i<cmd[2];i++)
        {
            if(sign->SignId()==*id)
            {
                sign->DispNext(DISP_STATUS::TYPE::ATF, *(id+1));
                break;
            }
            id+=2;
        }
    }
}

void Group::DispExtSw(uint8_t id)
{
    DispNext(DISP_STATUS::TYPE::Ext, id);
}

void Group::SetDimming(uint8_t dimming)
{
    for(auto sign : gUntSigns)
    {
        sign->SetDimming(dimming);
    }
}

void Group::SetPower(uint8_t v)
{
    if(v==0)
    {// set power off

    }
    else
    {// set power on

    }
    power=v;
}

void Group::SignSetPower(uint8_t v)
{
    for(auto sign : gUntSigns)
    {
        sign->SetPower(power);
    }
}

void Group::SetDevice(uint8_t endis)
{
    for(auto sign : gUntSigns)
    {
        sign->SetDevice(endis);
    }
}
