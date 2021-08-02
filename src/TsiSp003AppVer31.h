#ifndef __TSISP003APPVER31_H__
#define __TSISP003APPVER31_H__

#include "TsiSp003AppVer10.h"

class TsiSp003AppVer31 : public TsiSp003AppVer10
{
public:
    TsiSp003AppVer31();
    virtual ~TsiSp003AppVer31();
    virtual std::string Version() override { return "Ver3.1"; }

    virtual int Rx(uint8_t * data, int len) override;

private:
    void SignExtendedStatusRequest(uint8_t * data, int len);
    void SignSetGraphicsFrame(uint8_t * data, int len);
    void SignRequestStoredFMP(uint8_t * data, int len);
};

#endif
