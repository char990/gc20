#ifndef __ILOWERLAYER_H__
#define __ILOWERLAYER_H__

class ILowerLayer
{
public:
    enum class LowerLayerType
    {
        TSISP003LOWER,
        WEB2APPLOWER
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
