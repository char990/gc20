#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "gpio/GpioOut.h"

GpioOut *pPinCmdPower;
GpioOut *pPinHeartbeatLed;
GpioOut *pPinStatusLed;
GpioOut *pPinWdt;
GpioOut *pPinRelay;
GpioOut *pPinMosfet2;


GpioOut::GpioOut(unsigned int pin, bool init_value)
:gpioex(pin,GpioEx::DIR::OUTPUT)
{
    SetPin(init_value);
}

GpioOut::~GpioOut()
{
}

bool GpioOut::GetPin(void)
{
    return pin_v;
}

void GpioOut::SetPin(bool v)
{
    gpioex.SetValue(v);
    pin_v=v;
}

void GpioOut::SetPinHigh()
{
    SetPin(true);
}

void GpioOut::SetPinLow()
{
    SetPin(false);
}

void GpioOut::Toggle()
{
    pin_v ? SetPin(false) : SetPin(true);
}