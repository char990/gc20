#ifndef __UCIPROCESS_H__
#define __UCIPROCESS_H__

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

    // do not use
    GrpProc * GetGrpProc(uint8_t gid);

    bool IsPlanEnabled(uint8_t gid, uint8_t pid);
    void EnablePlan(uint8_t gid, uint8_t pid);
    void DisablePlan(uint8_t gid, uint8_t pid);

    void SetDisp(uint8_t gid, GrpProcDisp *disp);
    GrpProcDisp * GetDisp(uint8_t gid);

    void SetDimming(uint8_t gid, uint8_t v);
    uint8_t GetDimming(uint8_t gid);

private:
    GrpProc * grpProc;
    uint8_t grpCnt;

    int PrintGrpPln(uint8_t gid, char *buf);
    void SaveGrpPln(uint8_t gid);

    int PrintGrpFmpId(uint8_t gid, char *buf);

    char GroupX[8];

    const char * _EnabledPlan = "EnabledPlan";
    const char * _Display = "Display";
    const char * _FmpId = "FmpId";
    const char * _Dimming = "Dimming";
};

#endif
