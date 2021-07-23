#ifndef __TSISP003APPVER10_H__
#define __TSISP003APPVER10_H__

#include "TsiSp003Cfg.h"
#include "ITsiSp003App.h"

/// \brief  This is the base of TsiSp003 Application layer
class TsiSp003AppVer10 : public ITsiSp003App
{
public:
    TsiSp003AppVer10();
    ~TsiSp003AppVer10();

    virtual std::string Version() override { return std::string{"Ver1.0"}; };

    /// \brief  Callback handle of receiving data
    /// \param  data    data buffer
    /// \param  len     data len
    /// \return int     -1: No cmd matched
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief  Check and run new added MI rather than old revision
    /// \param  data    data buffer
    /// \param  len     data len
    /// \return int     -1: No cmd matched, call base Rx()
    virtual int NewMi(uint8_t * data, int len) override;
};


#endif
