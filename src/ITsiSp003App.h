#ifndef __ITSISP003App_H__
#define __ITSISP003App_H__

#include <string>
#include "IAdaptLayer.h"

class ITsiSp003App
{
public:
    ITsiSp003App();
    ~ITsiSp003App();

    virtual std::string Version()=0;

    /// \brief Receiving Handle, called by LowerLayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: unknown command
    virtual int Rx(uint8_t * data, int len)=0;

    /// \brief Transmitting function, call Tx() of lowerLayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual void SetLowerLayer(IAdaptLayer * ll)
    {
        adaptlayer=ll;
    }

protected:
    IAdaptLayer * adaptlayer;
};

#endif
