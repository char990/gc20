#ifndef __APPADAPTOR_H__
#define __APPADAPTOR_H__

#include <cstdint>
#include <string>
#include "IAppAdaptor.h"
#include "IOperator.h"

class AppAdaptor
{
public:
    AppAdaptor(std::string name, std::string aType, IOperator * iOperator);
    ~AppAdaptor();

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd);

    /// \brief Transmitting function, call Tx() of adaptor
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len);

    /// \brief		periodic run
    virtual void PeriodicRun();

private:
    IAppAdaptor * adaptor;
};

#endif
