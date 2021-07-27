#ifndef __TSISP003App_H__
#define __TSISP003App_H__

#include <string>
#include "ILayer.h"
#include "TsiSp003Cfg.h"
#include "TsiSp003AppVer31.h"
#include "TsiSp003AppVer50.h"

/// \brief TSiSp003 Application Layer agent
class TsiSp003App : public ILayer
{
public:
    TsiSp003App();
    ~TsiSp003App();

    /// \brief Transmitting function, called by upperlayer and call lowerlayer->Tx()
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len) override;

    /// \brief		Release current layer. Called by upperlayer and call lowerlayer->Release()
    virtual void Release() override;

    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief		Periodic run. Called by lowerlayer->PeriodicRun() and call upperlayer->PeriodicRun()
    virtual void PeriodicRun() override;

    /// \brief		Init current layer. Called by lowerlayer->Init() and call upperlayer->Init()
    virtual void Clean() override;

    /// \brief		Set lower layer
    virtual void LowerLayer(ILayer * lowerLayer) override
    {
        iAppLayer->LowerLayer(lowerLayer);
    }

    /// \brief		Set upper layer
    virtual void UpperLayer(ILayer * upperLayer) override
    {
        iAppLayer->UpperLayer(upperLayer);
    }

private:
    ILayer * iAppLayer;
};

#endif
