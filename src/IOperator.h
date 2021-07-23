#ifndef __IOPERATOR_H__
#define __IOPERATOR_H__

#include <string>
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

    /// \brief  Called only after object was created
    virtual void Init(std::string aType, std::string name)=0;

    /// \brief  Called when connection was accepted
    virtual void Setup(int fd)=0;

};

#endif