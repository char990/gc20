#pragma once

#include <string>
#include <vector>

#include <uci/UciCfg.h>
#include <uci/GrpProc.h>
#include <module/Utils.h>

class UciProcess : public UciCfg
{
public:
    UciProcess();
    ~UciProcess();

    void LoadConfig() override;

    void Dump() override;

    /******************** GroupX ***********************/
    bool IsPlanEnabled(uint8_t gid, uint8_t pid);
    void EnDisPlan(uint8_t gid, uint8_t pid, bool endis);

    void SetDisp(uint8_t gid, uint8_t *cmd, int len);
    /// \brief  [0]:len, [1]MI, [2]GroupID, [3]FrmId/MsgId/NumberOfSigns.....
    uint8_t *GetDisp(uint8_t gid);

    void SetDimming(uint8_t gid, uint8_t v);
    uint8_t GetDimming(uint8_t gid);

    void SetPower(uint8_t gid, uint8_t v);
    uint8_t GetPower(uint8_t gid);

    void SetDevice(uint8_t gid, uint8_t v);
    uint8_t GetDevice(uint8_t gid);

    /******************** Ctrller ***********************/
    Utils::Bool32 &CtrllerErr() { return ctrllerErr; };
    void SaveCtrllerErr(uint32_t v);

    /******************** SignX ***********************/
    Utils::Bool32 *SignErr()
    {
        return &signErr[0];
    };
    Utils::Bool32 *SignErr(uint8_t signId)
    {
        return (signId == 0 || signId > signCnt) ? nullptr : &signErr[signId - 1];
    };
    void SaveSignErr(uint8_t signId, uint32_t v);

private:
    GrpProc *grpProc;
    uint8_t grpCnt;
    GrpProc *GetGrpProc(uint8_t gid);

    int PrintGrpPln(uint8_t gid, char *buf);
    void SaveGrpPln(uint8_t gid);

    char sectionBuf[8];
    const char *_Group = "Group";
    const char *_EnabledPlan = "EnabledPlan";
    const char *_Display = "Display";
    const char *_Dimming = "Dimming";
    const char *_Power = "Power";
    const char *_Device = "Device";

    const char *_Ctrller = "Ctrller";
    Utils::Bool32 ctrllerErr;
    const char *_CtrllerError = "CtrllerError";

    const char *_Sign = "Sign";
    Utils::Bool32 *signErr;
    uint8_t signCnt;
    const char *_SignError = "SignError";
};
