#ifndef __TSISP003App_H__
#define __TSISP003App_H__

#include <string>
#include "ILayer.h"

/// \brief TSiSp003 Application Layer base
class TsiSp003App : public ILayer
{
public:
    TsiSp003App(bool & online);
    virtual ~TsiSp003App();

    virtual std::string Version()=0;

    /// \brief Transmitting function, called by upperlayer and call lowerlayer->Tx()
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len) override;

    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief		Release current layer. Called by upperlayer and call lowerlayer->Release()
    virtual void Release() override;

    /// \brief		Periodic run. Called by lowerlayer->PeriodicRun() and call upperlayer->PeriodicRun()
    virtual void PeriodicRun() override;

    /// \brief		Init current layer. Called by lowerlayer->Init() and call upperlayer->Init()
    virtual void Clean() override;

protected:
    bool & online;
};

#endif
