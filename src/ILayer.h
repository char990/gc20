#ifndef __ILAYER_H__
#define __ILAYER_H__

#include <cstdint>

class ILayer
{
public:
    ILayer():lowerLayer(nullptr),upperLayer(nullptr){};
    virtual ~ILayer(){};
    
    /// \brief		Set lower layer
    virtual void LowerLayer(ILayer * lowerLayer)
    {
        this->lowerLayer=lowerLayer;
    }

    /// \brief		Set upper layer
    virtual void UpperLayer(ILayer * upperLayer)
    {
        this->upperLayer=upperLayer;
    }

    /// \brief Transmitting function, called by upperlayer and call lowerlayer->Tx()
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len)=0;

    /// \brief		Release current layer. Called by upperlayer and call lowerlayer->Release()
    virtual void Release()=0;

    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len)=0;

    /// \brief		Periodic run. Called by lowerlayer->PeriodicRun() and call upperlayer->PeriodicRun()
    virtual void PeriodicRun()=0;

    /// \brief		Clean current layer. Called by lowerlayer->Clean() and call upperlayer->Clean()
    virtual void Clean()=0;
    
protected:
    ILayer * lowerLayer;
    ILayer * upperLayer;
};

#endif
