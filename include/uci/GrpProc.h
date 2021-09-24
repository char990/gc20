#pragma once


#include <cstdint>
#include <tsisp003/TsiSp003Const.h>

class GrpProcDisp
{
public:
    DISP_STATUS::TYPE dispType;
    uint8_t fmpLen;
    uint8_t fmpid[16];      // assume signs in group is less than 16
};

class GrpProc
{
public:
    GrpProc();
    bool IsPlanEnabled(uint8_t id);
    void EnablePlan(uint8_t id);
    void DisablePlan(uint8_t id);

    GrpProcDisp * ProcDisp() { return &procDisp; };

    void Dimming(uint8_t v) { dimming = v; };
    uint8_t Dimming() { return dimming; };
private:
    GrpProcDisp procDisp;
    uint8_t enabledPln[255];
    uint8_t dimming;
};