#ifndef __TSISP003APPVER31_H__
#define __TSISP003APPVER31_H__

#include "TsiSp003AppVer10.h"

class TsiSp003AppVer31 : public TsiSp003AppVer10
{
public:
    TsiSp003AppVer31();
    ~TsiSp003AppVer31();

    virtual int NewMi(uint8_t * data, int len);

    virtual int Rx(uint8_t * data, int len) override;

};

#endif
