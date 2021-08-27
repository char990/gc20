#ifndef __SIGNTXT_H__
#define __SIGNTXT_H__

#include <module/Sign.h>

class SignTxt : public Sign
{
public:
    virtual void PeriodicRun() override ;
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
