#ifndef __IADAPTLAYER_H__
#define __IADAPTLAYER_H__

#include <cstdint>

#include "TimerEvent.h"

/// \brief  Adapt layer is an adaptor between byte stream and appication layer
class IAppAdaptor : public IPeriodicEvent
{
public:
    IAppAdaptor(){};
    virtual ~IAppAdaptor(){};
    
    /// \brief		timer event manager
    static TimerEvent * tmrEvent;

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd)=0;
    
    /// \brief Transmitting function, call Tx() of adaptor
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len)=0;

    /// \brief		periodic run
    virtual void PeriodicRun() override =0;
};

#endif
