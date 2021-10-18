#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "gpio/GpioIn.h"
#include "module/MyDbg.h"

GpioIn *pFsAuto;
GpioIn *pFsM1;
GpioIn *pFsM2;
GpioIn *pExtM3;
GpioIn *pExtM4;
GpioIn *pExtM5;
GpioIn *pMainPwr;
GpioIn *pBatLow;
GpioIn *pBatOpen;

GpioIn::GpioIn(int true_cnt, int false_cnt, unsigned int pin)
    : dbnc(true_cnt, false_cnt), gpioex(pin, GpioEx::DIR::INPUT), pin(pin)
{
}

GpioIn::~GpioIn()
{
}

bool GpioIn::IsValid()
{
    return dbnc.IsValid();
}
Utils::STATE3 GpioIn::Value()
{
    return dbnc.Value();
}
bool GpioIn::IsChanged()
{
    return dbnc.changed;
}
void GpioIn::ClearChanged()
{
    dbnc.changed=false;
}

void GpioIn::PeriodicRun()
{
    int v = gpioex.GetValue();
    if (v != -1)
    {
        dbnc.Check(v!=0);
    }
    else
    {
        MyThrow("Read Pin[%d] failed", pin);
    }
}
