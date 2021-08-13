#ifndef __SIGNGFX_H__
#define __SIGNGFX_H__

#include <module/Sign.h>

class SignGfx : public Sign
{
public:
    virtual uint8_t * GetExtStatus(uint8_t *p) override;
};

#endif
