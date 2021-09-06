#ifndef __SIGNTXT_H__
#define __SIGNTXT_H__

#include <module/SignDisp.h>

class SignTxt : public SignDisp
{
public:
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
};

#endif
