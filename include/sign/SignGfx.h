#ifndef __SIGNGFX_H__
#define __SIGNGFX_H__

#include <sign/Sign.h>

class SignGfx : public Sign
{
public:
    SignGfx(uint8_t signId):Sign(signId){};
    ~SignGfx(){};
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
