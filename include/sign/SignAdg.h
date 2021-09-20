#ifndef __SIGNHRG_H__
#define __SIGNHRG_H__

#include <sign/SignGfx.h>

class SignAdg : public SignGfx
{
public:
    SignAdg(uint8_t signId);
    ~SignAdg();
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
