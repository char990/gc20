#ifndef __SIGNHRG_H__
#define __SIGNHRG_H__

#include <sign/SignGfx.h>

class SignHrg : public SignGfx
{
public:
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
