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

extern GpioIn *pFsAuto;
extern GpioIn *pFsM1;
extern GpioIn *pFsM2;
extern GpioIn *pExtM3;
extern GpioIn *pExtM4;
extern GpioIn *pExtM5;
extern GpioIn *pMainPwr;
extern GpioIn *pBatLow;
extern GpioIn *pBatOpen;
