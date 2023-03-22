#pragma once

#include <cstdint>

class IUpperLayer;
class ILowerLayer;

class IUpperLayer
{
public:
    IUpperLayer() : lowerLayer(nullptr){};
    virtual ~IUpperLayer(){};
    /// \brief		Set lower layer
    virtual void LowerLayer(ILowerLayer *lowerLayer)
    {
        this->lowerLayer = lowerLayer;
    }
    /// \brief Receiving Handle, called by lowerlayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t *data, int len) = 0;

    /// \brief		Clear current layer. Called by lowerlayer
    virtual void ClrRx() = 0;

protected:
    ILowerLayer *lowerLayer{nullptr};
};

class ILowerLayer
{
public:
    ILowerLayer() : upperLayer(nullptr){};
    virtual ~ILowerLayer(){};

    /// \brief		Set upper layer
    virtual void UpperLayer(IUpperLayer *upperLayer)
    {
        this->upperLayer = upperLayer;
    }

    /// \brief Chekc if Tx() is ready, called by upperlayer before calling Tx()
    virtual bool IsTxReady() = 0;

    /// \brief Transmitting function, called by upperlayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t *data, int len) = 0;

    /// \brief  Stop TX and Clear current layer. Called by lowerlayer
    virtual void ClrTx() = 0;

    /// \brief  Printf Rx buffer data for debug
    virtual void PrintRxBuf() = 0;

protected:
    IUpperLayer *upperLayer{nullptr};
};

class ILayer : public IUpperLayer, public ILowerLayer
{
public:
    ILayer(){};
    virtual ~ILayer(){};
};

