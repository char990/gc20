#pragma once

#include <gpio/GpioIn.h>

class FacilitySwitch
{
public:
    enum FS_STATE {OFF, AUTO,MSG1, MSG2};
    FacilitySwitch(uint32_t pinAuto, uint32_t pinM1, uint32_t pinM2);
    ~FacilitySwitch();
    void PeriodicRun();

    bool IsChanged() { return isChanged; };
    void ClearChangeFlag() { isChanged = false; };

    FS_STATE Get() { return fsState; };

private:
    bool isChanged{false};
    FS_STATE fsState{FS_STATE::OFF};
    GpioIn *pFsAuto;
    GpioIn *pFsM1;
    GpioIn *pFsM2;
    uint8_t cnt;
};
