#ifndef __TSISP003APPVER50_H__
#define __TSISP003APPVER50_H__

#include <vector>
#include "TsiSp003AppVer31.h"
#include "BootTimer.h"

class TsiSp003AppVer50: TsiSp003AppVer31
{
public:
    TsiSp003AppVer50();
    ~TsiSp003AppVer50();

    virtual std::string Version() override;

    virtual int Received(uint8_t * data, int len) override;

private:
};

#endif
