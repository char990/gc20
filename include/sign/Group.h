#pragma once


#include <vector>
#include <module/OprSp.h>
#include <module/OprTcp.h>
#include <sign/Sign.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/FacilitySwitch.h>
#include <module/ExtInput.h>
#include <module/BootTimer.h>
#include <sign/DispStatus.h>
#include <layer/ILayer.h>

class PlnMinute
{
public:
    uint8_t type;   // 0:Empty, 1:Frm, 2:Msg
    uint8_t id;
};

class Group : public IUpperLayer
{
public:
    Group(uint8_t groupId);
    virtual ~Group();

    /*<------------------------------------------------------------------*/
    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief		Clean current layer. Called by lowerlayer->Clean() and call upperlayer->Clean()
    virtual void Clean() override;
    /*------------------------------------------------------------------>*/

    /// \brief call lowerlayer->Tx
    virtual int Tx(uint8_t * data, int len) { return lowerLayer->Tx(data, len); }

    // called by scheduler
    void PeriodicRun();

    // child class PeriodicRun
    virtual void PeriodicHook()=0;

    //Getter
    uint8_t GroupId() { return groupId; };
    int SignCnt() { return vSigns.size(); };
    Sign* GetSign(uint8_t id);

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
    APP::ERROR EnablePlan(uint8_t id);
    // if planid==0, disable all plans
    APP::ERROR DisablePlan(uint8_t id);

    APP::ERROR DispFrm(uint8_t id);
    APP::ERROR DispMsg(uint8_t id);
    virtual APP::ERROR DispAtomicFrm(uint8_t *id) = 0;

    APP::ERROR SetDimming(uint8_t v);
    APP::ERROR SetPower(uint8_t v);
    APP::ERROR SetDevice(uint8_t v);

    void DispExtSw(uint8_t id);

protected:
    uint8_t groupId;
    std::vector<Sign*> vSigns;
    std::vector<Slave*> vSlaves;

    // display status
    DispStatus *dsBak;
    DispStatus *dsCurrent;
    DispStatus *dsNext;
    DispStatus *dsExt;
    uint8_t msgEnd;

    // group status
    enum PWR_STATE {OFF, ON, RISING};
    PWR_STATE power;

    // for Display command except for ATF
    void DispNext(DISP_STATUS::TYPE type,uint8_t id);

    void DispBackup();

    void SignSetPower(uint8_t v);

    BootTimer pwrUpTmr;

    FacilitySwitch fcltSw;
    ExtInput    extInput;

    void FcltSwitchFunc();
    void ExtInputFunc();

    PlnMinute plnMin[7*24*60];
    
    bool LoadDsNext();

    bool IsDsNextEmergency();

    int taskPlnLine;
    BootTimer task1Tmr;
    bool TaskPln(int * _ptLine);

    int taskMsgLine;
    BootTimer task2Tmr;
    bool TaskMsg(int * _ptLine);

private:
    void SlaveStatusRpl(uint8_t * data, int len);
    void SlaveExtStatusRpl(uint8_t * data, int len);

};

