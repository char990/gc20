#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "gpio/GpioIn.h"
#include "module/MyDbg.h"


GpioIn::GpioIn(int true_cnt, int false_cnt, unsigned int pin)
    : Debounce(true_cnt, false_cnt), gpioex(pin, GpioEx::DIR::INPUT), pin(pin)
{
}

GpioIn::~GpioIn()
{
}

void GpioIn::PeriodicRun()
{
    int v = gpioex.GetValue();
    if (v != -1)
    {
        Check(v!=0);
    }
    else
    {
        MyThrow("Read Pin[%d] failed", pin);
    }
}
