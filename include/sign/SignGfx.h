#ifndef __SIGNGFX_H__
#define __SIGNGFX_H__

#include <sign/SignTxt.h>

class SignGfx : public SignTxt
{
public:
    virtual void PeriodicRun() override;
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
