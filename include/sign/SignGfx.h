#pragma once

#include <sign/Sign.h>

class SignGfx : public Sign
{
public:
    SignGfx(uint8_t signId) : Sign(signId){};
    ~SignGfx(){};
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
    virtual uint8_t *GetConfig(uint8_t *p) override;
};
