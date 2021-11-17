#pragma once

#include <gpio/GpioIn.h>

#define FacilitySwitch_BUF_SIZE 32 
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

    char * ToStr() { return fsbuf; };

private:
    bool isChanged{false};
    FS_STATE fsState{FS_STATE::NA};
    GpioIn *pFsAuto;
    GpioIn *pFsM1;
    GpioIn *pFsM2;
    FS_STATE lastState{FS_STATE::NA};
    const char * FS_STR[5]{"N/A", "OFF", "AUTO", "MSG1", "MSG2"};
    uint8_t lastkey{0};
    uint8_t keyCnt{0};
    char fsbuf[FacilitySwitch_BUF_SIZE];
};
