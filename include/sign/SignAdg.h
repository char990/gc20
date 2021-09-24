#pragma once


#include <sign/Sign.h>

class SignAdg : public Sign
{
public:
    SignAdg(uint8_t signId):Sign(signId){};
    ~SignAdg(){};
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

