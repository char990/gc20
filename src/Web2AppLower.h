#ifndef __WEB2APPLOWER_H__
#define __WEB2APPLOWER_H__
#include <string>
#include "ILowerLayer.h"
#include "IOperator.h"

class Web2AppLower : public ILowerLayer
{
public:
    Web2AppLower(std::string name);
    /// \brief		periodic run
    void PeriodicRun();

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd);
    
    /// \brief Transmitting function, call Tx() of operator
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len);

private:
    std::string name;
    IOperator * operator;
};

#endif
