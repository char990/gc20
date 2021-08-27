#ifndef __SIGNHRG_H__
#define __SIGNHRG_H__

#include <module/Sign.h>

class SignHrg : public Sign
{
public:
    virtual void PeriodicRun() override ;
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
