#ifndef __TSISP003APPVER50_H__
#define __TSISP003APPVER50_H__

#include "TsiSp003AppVer31.h"

class TsiSp003AppVer50 : public TsiSp003AppVer31
{
public:
    TsiSp003AppVer50(bool & online);
    virtual ~TsiSp003AppVer50();
    virtual std::string Version() override { return "Ver5.0"; }

    virtual int NewMi(uint8_t * data, int len) override;

    virtual int Rx(uint8_t * data, int len) override;

private:

};

#endif
