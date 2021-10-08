#pragma once
#include <cstring>
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
    uint8_t plnId{0};   // 0:Empty, 1-255 pln Id
    uint8_t fmType{0};    // 1:Frm, 2:Msg
    uint8_t fmId{0};    // frm/msg Id
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

    /// \brief      call TranslateFrame and send txBuf by SetTextFrame/SetGfxFrm
    int SlaveSetFrame(uint8_t slvindex, uint8_t slvFrmId, uint8_t uciFrmId);

    int SlaveDisplayFrame(uint8_t slvindex, uint8_t slvFrmId);
    int SlaveSetStoredFrame(uint8_t slvindex, uint8_t slvFrmId);

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
    APP::ERROR EnDisPlan(uint8_t id, bool endis);
    APP::ERROR EnPlan(uint8_t id);
    APP::ERROR DisPlan(uint8_t id);

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

    bool AllSlavesGotStatus();
    void ClrAllSlavesRxStatus();
    bool AllSlavesGotExtSt();
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

    int taskATFLine{0};
    virtual bool TaskSetATF(int *_ptLine)=0;

    virtual void TransFrmToOrBuf(uint8_t frmId)=0;
    virtual void MakeFrameForSlave(uint8_t fid)=0;
    uint8_t msgOverlay{0}; // 0:No overlay, 1:mono gfx, 4:4-bit gfx, 24:24-bit gfx
    int orLen;
    uint8_t *orBuf;
    void ClrOrBuf()
    {
        msgOverlay=0;
        memset(orBuf,0,orLen);
    }

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
    PlnMinute & GetCurrentMinPln();
    int GetMinOffset(int day, Hm * t);
    void LoadPlanToPlnMin(uint8_t id);
    void PrintPlnMin();
    bool LoadDsNext();

    bool IsDsNextEmergency();

    /******************** Task Plan ********************/
    uint8_t
        newPlnId,
        plnEntryType, // 1:frm, 2:msg
        plnEntryId;

    int taskPlnLine{0};
    BootTimer taskPlnTmr;
    bool TaskPln(int *_ptLine);
    void TaskPlnReset()
    {
        taskPlnLine=0;
    }

    /******************** Task Message ********************/
    uint8_t
        newMsg,         // 0:EMPTY, 1:new msg load
        newMsgId,
        msgEntryCnt;   // 0 - (msg->entries-1)
    // msgSetEntry/Max depend on frame 'onTime'=0(frame overlay)
    // if there is an entry onTime(!0) following an entry onTime(0), msgSetEntryMax = msg->entries + last onTime(0) entry
    // otherwise, msgSetEntryMax=msg->entries
    // E.g.
    //  6 frames in msg(entry[0-5]) and entry[0].onTime=50, entry[1].onTime=0
    //  msgSetEntryMax = 6 + 1;
    //  first round:
    //      set [0]                 :1
    //      [1] set in orBUf        :2
    //      set [2-5] with orBuf    :3-6
    //      set [0] with orBuf      :7
    // E.g.
    //  4 frames in msg(entry[0-3]) and entry[0-2].onTime=50, entry[3].onTime=0
    //  msgSetEntryMax = 4;
    //  first round:
    //      set [0-2]               :1-3
    //      set [3] (last entry)    :4
    // E.g.
    //  4 frames in msg(entry[0-3]) and entry[0-1].onTime=0, entry[2-3].onTime=50
    //  msgSetEntryMax = 4;
    //  first round:
    //      [0-1] set in orBUf      :1-2
    //      set [2-3]  with orBuf   :3-4
    uint8_t
        msgSetEntry,     // entry counter to set frame
        msgSetEntryMax;  // max netries to set frame
    Message * pMsg;
    int taskMsgLine{0};
    BootTimer taskMsgTmr;
    BootTimer taskMsgLastFrmTmr;
    bool TaskMsg(int *_ptLine);
    void TaskMsgReset()
    {
        taskMsgLine=0;
        msgEntryCnt=0;
        msgSetEntryMax=0;
        msgSetEntry=0;
    }
    void InitMsgOverlayBuf(Message * pMsg);    

    /******************** Task Frame ********************/
    uint8_t
        newFrm,   // 0:EMPTY, 1:new frm load
        newFrmId; // if frmId is 0, BLANK, this is for dispFrm0 and no valid plan

    bool TaskFrm(int *_ptLine);
    int taskFrmLine{0};
    BootTimer taskFrmTmr;
    void TaskFrmReset()
    {
        taskFrmLine=0;
        taskATFLine=0;
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

    void GroupSetReportDisp(uint8_t newFrmId, uint8_t newMsgId, uint8_t newPlnId);

    bool IsDimmingChanged();

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
    
    Utils::Bool256 activeFrm;

    Utils::Bool256 activeMsg;
    // will set activeMsg & activeFrm
    void SetActiveMsg(uint8_t mid);
};
