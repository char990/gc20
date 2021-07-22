#ifndef __IADAPTLAYER_H__
#define __IADAPTLAYER_H__

#include <cstdint>

/// \brief  Adapt layer is an adaptor between byte stream and appication layer
class IAdaptLayer
{
public:
    enum class AdType
    {
        /// \brief  AdaptorType-TsiSp003  
        AT_TSI,
        /// \brief  AdaptorType-Web2App  
        AT_W2A
    };

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd)=0;
    
    /// \brief Transmitting function, call Tx() of lowerLayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len)=0;
};

#endif
