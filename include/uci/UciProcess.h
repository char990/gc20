#pragma once

#include <string>
#include <uci/UciCfg.h>
#include <uci/GrpProc.h>

class UciProcess : public UciCfg
{
public:
    UciProcess();
    ~UciProcess();

    void LoadConfig() override;

	void Dump() override;

    bool IsPlanEnabled(uint8_t gid, uint8_t pid);
    void EnDisPlan(uint8_t gid, uint8_t pid, bool endis);

    void SetDisp(uint8_t gid, uint8_t *cmd, int len);
    /// \brief  [0]:len, [1]MI, [2]GroupID, [3]FrmId/MsgId/NumberOfSigns..... 
    uint8_t * GetDisp(uint8_t gid);

    void SetDimming(uint8_t gid, uint8_t v);
    uint8_t GetDimming(uint8_t gid);

    void SetPower(uint8_t gid, uint8_t v);
    uint8_t GetPower(uint8_t gid);

    void SetDevice(uint8_t gid, uint8_t v);
    uint8_t GetDevice(uint8_t gid);

private:
    GrpProc * grpProc;
    uint8_t grpCnt;
    GrpProc *GetGrpProc(uint8_t gid);

    int PrintGrpPln(uint8_t gid, char *buf);
    void SaveGrpPln(uint8_t gid);

    char GroupX[8];

    const char * _EnabledPlan = "EnabledPlan";
    const char * _Display = "Display";
    const char * _Dimming = "Dimming";
    const char * _Power = "Power";
    const char * _Device = "Device";
};
