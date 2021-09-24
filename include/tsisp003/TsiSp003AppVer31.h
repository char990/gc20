#pragma once


#include <tsisp003/TsiSp003AppVer21.h>

class TsiSp003AppVer31 : public TsiSp003AppVer21
{
public:
    TsiSp003AppVer31();
    virtual ~TsiSp003AppVer31();
    virtual std::string Version() override { return "Ver3.1"; }

    virtual int Rx(uint8_t * data, int len) override;

private:
};

