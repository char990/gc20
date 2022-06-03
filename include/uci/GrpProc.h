#pragma once


#include <cstdint>
#include <tsisp003/TsiSp003Const.h>
#include <module/Utils.h>

class GrpProc
{
public:
    GrpProc(){};
    ~GrpProc(){};

    bool IsPlanEnabled(uint8_t id);
    void EnDisPlan(uint8_t id, bool endis);
    Utils::Bits & EnabledPln() { return enabledPln; };

    void ProcDisp(uint8_t *cmd, int len);
    uint8_t * ProcDisp() { return disp; };

    void Dimming(uint8_t v) { dimming = v; };
    uint8_t Dimming() { return dimming; };

    void Power(uint8_t v) { power = v; };
    uint8_t Power() { return power; };

    void DeviceOnOff(uint8_t v) { device = v; };
    uint8_t DeviceOnOff() { return device; };

private:
    Utils::Bits enabledPln{256};
    uint8_t dimming{0};
    uint8_t power{1};
    uint8_t device{1};
    uint8_t disp[256]{};      // [0]:len, [1-255]:display cmd
};
