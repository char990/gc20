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
    virtual void Rx()=0;
    
    /// \brief		Send data
    virtual void Tx()=0;
};

#endif
