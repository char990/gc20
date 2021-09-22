#ifndef __GROUP_H__
#define __GROUP_H__

#include <vector>
#include <sign/Sign.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/FacilitySwitch.h>
#include <module/BootTimer.h>

class DispStatus
{
public:
    DispStatus(uint8_t signCnt)
    :signCnt(signCnt)
    {
        dispType = DISP_STATUS::TYPE::N_A;
        fmpid = new uint8_t[signCnt];
    };
    ~DispStatus()
    {
        delete [] fmpid;
    };
    void Clone(DispStatus * ds)
    {
        dispType = ds->dispType;
        for(int i=0;i<signCnt;i++)
        {
            fmpid[i] = ds->fmpid[i];
        }
    };
    void Frm0()
    {
        dispType = DISP_STATUS::TYPE::FRM;
        fmpid[0] = 0;
    };
    void N_A()
    {
        dispType = DISP_STATUS::TYPE::N_A;
        fmpid[0] = 0;
    };
    DISP_STATUS::TYPE dispType;
    uint8_t *fmpid;
    uint8_t signCnt;
};


class PlnMinute
{
public:
    uint8_t type;
    uint8_t id;
};

class Group
{
public:
    Group(uint8_t groupId);
    virtual ~Group();

    // Add a sign into this group
    void Add(Sign * sign);

    // After all signs added, init group
    void Init();

    // called by scheduler
    void PeriodicRun();

    // child class PeriodicRun
    virtual void PeriodicHook()=0;

    //Getter
    uint8_t GroupId() { return groupId; };
    uint8_t SignCnt() { return signCnt; };

    // signs in group
    bool IsSignInGroup(uint8_t id);

    // if planid==0, check any plan
    bool IsPlanActive(uint8_t id);

    // id could not be 0
    bool IsPlanEnabled(uint8_t id);
    bool IsEnPlanOverlap(uint8_t id);
    bool IsMsgActive(uint8_t id);
    bool IsFrmActive(uint8_t id);

    /********* command from Tsi-sp-003 ********/
    // if planid==0, disable all plans
    void EnablePlan(uint8_t id);
    // if planid==0, disable all plans
    void DisablePlan(uint8_t id);

    void DispFrm(uint8_t id);
    void DispMsg(uint8_t id);
    void DispAtomicFrm(uint8_t *id);

    void SetDimming(uint8_t v);
    void SetPower(uint8_t v);
    void SetDevice(uint8_t v);

protected:
    uint8_t groupId;
    std::vector<Sign *> vSigns;
    uint8_t signCnt;

    // display status
    DispStatus *dsBak;
    DispStatus *dsCurrent;
    DispStatus *dsNext;
    DispStatus *dsExt;

    // group status
    enum PWR_STATE {OFF, ON, RISING};
    POWERSTATE power;

    // for Display command except for ATF
    void DispNext(DISP_STATUS::TYPE type,uint8_t id);

    void DispBackup();

    void SignSetPower(uint8_t v);

private:
    BootTimer pwrUpTmr;

    FacilitySwitch fcltSw;
    ExtInput    extInput;

    void FcltSwitch();
    void ExtInput();

    PlnMinute plnMin[7*24*60];
};

#endif
