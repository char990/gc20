#pragma once


#include <sign/Sign.h>

class SignTxt : public Sign
{
public:
    SignTxt(uint8_t signId):Sign(signId){};
    ~SignTxt(){};
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
};


