#ifndef __SIGNHRG_H__
#define __SIGNHRG_H__

#include "Sign.h"

class SignHrg : public Sign
{
public:
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
