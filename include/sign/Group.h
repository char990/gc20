#ifndef __GROUP_H__
#define __GROUP_H__

#include <vector>
#include <sign/UnitedSign.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/FacilitySwitch.h>
#include <module/BootTimer.h>


class Group
{
public:
    Group(uint8_t groupId);
    ~Group();

    // called by scheduler
    void PeriodicRun();

    //Getter
    uint8_t GroupId() { return groupId; };

    // Add a sign into this group
    void Add(UnitedSign * sign);

    // signs in group
    uint8_t NumberOfSigns() { return gUntSigns.size(); };
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
    void DispAtomicFrm(uint8_t *cmd, int len);

    void SetDimming(uint8_t dimming);
    void SetPower(uint8_t onoff);
    void SetDevice(uint8_t endis);

private:
    std::vector<UnitedSign *> gUntSigns;

    uint8_t groupId;
    enum PWR_STATE {OFF, ON, RISING};
    POWERSTATE power;

    void DispNext(DISP_STATUS::TYPE type,uint8_t id);
    void DispBackup();
    FacilitySwitch fcltSw;
    BootTimer pwrUpTmr;


    void SignSetPower(uint8_t v);
    void FcltSwitch();
    void ExtInput();

};

#endif
