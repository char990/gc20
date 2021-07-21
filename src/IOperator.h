#ifndef __IOPERATOR_H__
#define __IOPERATOR_H__

#include "IGcEvent.h"

class IOperator : public IGcEvent 
{
public:
    virtual void EventsHandle(uint32_t events)=0;

    /// \brief Transmitting function, call lower Tx() 
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len)=0;
};

#endif
