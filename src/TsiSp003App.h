#ifndef __TSISP003App_H__
#define __TSISP003App_H__

#include <string>
#include "ITsiSp003App.h"
#include "TsiSp003Cfg.h"
#include "TsiSp003AppVer31.h"
#include "TsiSp003AppVer50.h"

class TsiSp003App
{
public:
    TsiSp003App();
    ~TsiSp003App();

    //virtual std::string Version() override =0;

    /// \brief Receiving Handle, called by LowerLayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: unknown command
    virtual int Rx(uint8_t * data, int len);// override;

    /// \brief Transmitting function, call Tx() of adaptor
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len);

protected:
    ITsiSp003App * app;
    IAppAdaptor * adaptor;
};

#endif
