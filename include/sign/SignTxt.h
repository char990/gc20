#ifndef __SIGNTXT_H__
#define __SIGNTXT_H__

#include <sign/Sign.h>

class SignTxt : public Sign
{
public:
    SignTxt(uint8_t signId);
    ~SignTxt();
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
};

#endif
