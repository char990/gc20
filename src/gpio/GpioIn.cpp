#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "gpio/GpioIn.h"
#include "module/MyDbg.h"
#include <module/Utils.h>


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
        throw std::runtime_error(Utils::StrFn::PrintfStr("Read Pin[%d] failed", pin));
    }
}
