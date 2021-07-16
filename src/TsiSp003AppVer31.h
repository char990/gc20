#ifndef __TSISP003APPVER31_H__
#define __TSISP003APPVER31_H__

#include <vector>
#include "TsiSp003Cfg.h"
#include "TsiSp003App.h"
#include "BootTimer.h"

class TsiSp003AppVer31:TsiSp003App
{
public:
    TsiSp003AppVer31();
    ~TsiSp003AppVer31();

    virtual std::string Version() override;

    virtual int Received(uint8_t * data, int len) override;

private:

};

#endif
