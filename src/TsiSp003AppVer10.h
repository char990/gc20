#ifndef __TSISP003APPVER10_H__
#define __TSISP003APPVER10_H__

#include "TsiSp003Cfg.h"
#include "TsiSp003App.h"

/// \brief  This is the base of TsiSp003 Application layer
class TsiSp003AppVer10 : public TsiSp003App
{
public:
    TsiSp003AppVer10(bool & online);
    virtual ~TsiSp003AppVer10();

    virtual std::string Version() override { return "Ver1.0"; }

    virtual int Rx(uint8_t * data, int len) override;
};

#endif
