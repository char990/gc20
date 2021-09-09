#ifndef __SIGNHRG_H__
#define __SIGNHRG_H__

#include <sign/SignGfx.h>

class SignHrg : public SignGfx
{
public:
    SignHrg(uint8_t signId);
    ~SignHrg();
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
