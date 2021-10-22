#pragma once

#include <gpio/GpioIn.h>

class FacilitySwitch
{
public:
    enum FS_STATE {NA, OFF, AUTO, MSG1, MSG2};

    FacilitySwitch(uint32_t pinAuto, uint32_t pinM1, uint32_t pinM2);
    ~FacilitySwitch();
    void PeriodicRun();

    bool IsChanged() { return isChanged; };
    void ClearChanged() { isChanged = false; };

    FS_STATE Get() { return fsState; };

    char * ToStr() { return buf; };

private:
    bool isChanged{false};
    FS_STATE fsState{FS_STATE::NA};
    GpioIn *pFsAuto;
    GpioIn *pFsM1;
    GpioIn *pFsM2;
    uint8_t cnt;
    FS_STATE lastState{FS_STATE::NA};
    const char * FS_STR[5]{"N/A", "OFF", "AUTO", "MSG1", "MSG2"};
    uint8_t lastkey{0};
    uint8_t keyCnt{0};
    char buf[32];
};
