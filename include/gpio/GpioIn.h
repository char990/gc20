#pragma once

#include "gpio_def.h"

#include "gpio/GpioEx.h"
#include "module/Debounce.h"
#include "module/IPeriodicRun.h"
#include "module/Utils.h"

class GpioIn : public IPeriodicRun
{
public:
    GpioIn(int true_cnt,  int false_cnt, unsigned int pin);
    ~GpioIn(void);
    void Init(Utils::STATE3 s);
    bool IsValid(void);
 	Utils::STATE3 Value(void);
	bool IsChanged(void);
    void ClearChanged(void);

    virtual void PeriodicRun() override; // read pin

private:
    GpioEx gpioex;
    Debounce dbnc;
    unsigned int pin;
};

