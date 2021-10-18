#pragma once

#include <gpio/GpioIn.h>
class ExtInput
{
public:
    enum EXT_STATE {NONE, M3, M4, M5};
    ExtInput(uint32_t pinM3,uint32_t pinM4,uint32_t pinM5);
    ~ExtInput();
    void PeriodicRun();

    bool IsChanged() { return isChanged; };
    void ClearChangeFlag() { isChanged = false; };

    void Reset();
    
    EXT_STATE Get() { return state; };

private:
    bool isChanged{false};
    EXT_STATE state{EXT_STATE::NONE};
    GpioIn *pExtM3;
    GpioIn *pExtM4;
    GpioIn *pExtM5;
    uint8_t cnt;
};


