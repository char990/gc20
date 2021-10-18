#pragma once

#include "gpio_def.h"

#include "GpioEx.h"

class GpioOut
{
public:
    GpioOut(unsigned int pin, bool init_value);
    ~GpioOut(void);
    void SetPin(bool);
    void SetPinHigh(void);
    void SetPinLow(void);
    void Toggle();
    bool GetPin(void);

private:
    GpioEx gpioex;
    bool pin_v;
};

extern GpioOut * pPinCmdPower;
extern GpioOut * pPinHeartbeatLed;
extern GpioOut * pPinStatusLed;
extern GpioOut * pPinWdt;
extern GpioOut * pPinRelay;
extern GpioOut * pPinMosfet2;
