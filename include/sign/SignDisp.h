#ifndef __SIGNDISP_H__
#define __SIGNDISP_H__

class SignDisp
{
    SignDisp();
    virtual ~SignDisp();
    virtual uint8_t *GetExtStatus(uint8_t *p) = 0;
};

#endif
