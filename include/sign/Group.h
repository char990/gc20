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
#include <uci/DbHelper.h>
#include <module/Utils.h>

class PlnMinute
{
public:
    uint8_t type{0}; // 0:Empty, 1:Frm, 2:Msg
    uint8_t id{0};
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
    virtual int Rx(uint8_t *data, int len) override;

    /// \brief		ClrRx current layer. Called by lowerlayer->ClrRx() and call upperlayer->ClrRx()
    virtual void ClrRx() override;
    /*------------------------------------------------------------------>*/

    /// \brief send txBuf,txLen to lowerlayer->Tx
    int Tx();
    int RqstStatus(uint8_t slvindex);
    int RqstExtStatus(uint8_t slvindex);

    /// \brief      translate a frame to txBuf
    /// \param      frmId is the frame id in UciFrame, should be defined and not be 0
    ///             total bytes in txLen
    void TranslateFrame(uint8_t uciFrmId);

    /// \brief      call TranslateFrame and send txBuf by SetTextFrame/SetGfxFrm
    int SetFrame(uint8_t slvindex, uint8_t slvFrmId, uint8_t uciFrmId);

    int DisplayFrame(uint8_t slvindex, uint8_t slvFrmId);
    int SetStoredFrame(uint8_t slvindex, uint8_t slvFrmId);

    // called by scheduler
    void PeriodicRun();

    // child class PeriodicRun
    virtual void PeriodicHook() = 0;

    //Getter
    uint8_t GroupId() { return groupId; };
    int SignCnt() { return vSigns.size(); };
    Sign *GetSign(uint8_t id);
    int SlaveCnt() { return vSlaves.size(); };
    Slave *GetSlave(uint8_t id);

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

    //
    void DispExtSw(uint8_t id);

    int CmdToSlave(uint8_t id, uint8_t *data, int len); // to slave[id]
    int CmdToSlave(uint8_t *data, int len);             // broadcast

protected:
    DbHelper &db;
    uint8_t groupId;
    std::vector<Sign *> vSigns;
    std::vector<Slave *> vSlaves;

    int maxTxSize;
    uint8_t *txBuf;
    int txLen;

    void SlaveStatusRpl(uint8_t *data, int len);
    void SlaveExtStatusRpl(uint8_t *data, int len);

    bool CheckAllSlavesRxStatus();
    void ClrAllSlavesRxStatus();
    bool CheckAllSlavesRxExtSt();
    void ClrAllSlavesRxExtSt();

    Utils::STATE3 CheckAllSlavesNext();
    Utils::STATE3 allSlavesNext;
    Utils::STATE3 CheckAllSlavesCurrent();
    Utils::STATE3 allSlavesCurrent;

    void RefreshSignByStatus();
    void RefreshSignByExtSt();

    // display status
    DispStatus *dsBak;
    DispStatus *dsCurrent;
    DispStatus *dsNext;
    DispStatus *dsExt;

private:

    uint8_t readyToLoad{1}, newCurrent{0};

    enum
    {
        DEV_DIS = 0,
        DEV_EN = 1,
        DEV_WAIT = 2
    };
    uint8_t
        devSet, // DEV_DIS/DEV_EN
        devCur; // DEV_WAIT: dev set as disabled, but blank not sent out

    enum
    {
        typeEMPTY = 0,
        typeFRM = 1,
        typeMSG = 2,
        typeTRS = 255 // transition time in MSG
    };

    uint8_t         // new plan entry
        newPlnFM,
        plnEntryType, // 0:EMPTY, 1:frm, 2:msg
        plnEntryId;

    uint8_t
        newFrm,   //  0:EMPTY, 1:new frm load
        newFrmId; // if frmId is 0, BLANK

    // group status
    enum PWR_STATE
    {
        OFF,
        ON,
        RISING
    };
    PWR_STATE power;

    // for Display command except for ATF
    void DispNext(DISP_STATUS::TYPE type, uint8_t id);

    void DispBackup();

    void SignSetPower(uint8_t v);

    BootTimer pwrUpTmr;

    FacilitySwitch fcltSw;
    ExtInput extInput;

    void FcltSwitchFunc();
    void ExtInputFunc();

    PlnMinute plnMin[7 * 24 * 60]{};

    bool LoadDsNext();

    bool IsDsNextEmergency();

    int taskPlnLine{0};
    BootTimer taskPlnTmr;
    bool TaskPln(int *_ptLine);
    void TaskPlnReset()
    {
        taskPlnLine=0;
    }

    int taskMsgLine{0};
    BootTimer taskMsgTmr;
    bool TaskMsg(int *_ptLine);
    void TaskMsgReset()
    {
        taskMsgLine=0;
    }

    bool TaskFrm(int *_ptLine);
    int taskFrmLine{0};
    BootTimer taskFrmTmr;
    uint8_t frmFor;
    void TaskFrmReset()
    {
        taskFrmLine=0;
    }

    bool TaskRqstSlave(int *_ptLine);
    int taskRqstSlaveLine{0};
    BootTimer taskRqstSlaveTmr;
    uint8_t rqstStCnt{0};
    uint8_t rqstExtStCnt{0};
    uint8_t rqstNoRplCnt{0};
    void TaskRqstSlaveReset()
    {
        taskRqstSlaveLine=0;
        rqstStCnt=0;
        rqstExtStCnt=0;
        rqstNoRplCnt=0;
        taskRqstSlaveTmr.Setms(0);
    }

    bool IsDimmingChanged();

    uint8_t orType{0}; // 0:None, 1:mono, 4:4-bit, 24:24-bit
    int orLen;
    uint8_t *orBuf;

    enum SLVCMD
    {
        RQST_STATUS = 0x05,
        RPL_STATUS = 0x06,
        RQST_EXT_ST = 0x07,
        RPL_EXT_ST = 0x08,
        SET_TXT_FRM = 0x0A,
        SET_GFX_FRM = 0x0B,
        DISPLAY_FRM = 0x0E,
        SET_STD_FRM = 0x0F
    };

    BootTimer busLockTmr;
    bool IsBusFree();
    void LockBus(int ms);
};
