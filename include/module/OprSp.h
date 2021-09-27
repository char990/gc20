#pragma once


#include <module/SerialPort.h>
#include <module/IOperator.h>
#include <layer/ILayer.h>

/// \brief  Operator from serial port
class OprSp : public IOperator
{
public:
    OprSp(uint8_t comX, int bps, IUpperLayer * upperLayer);
    ~OprSp();

    const char* Name() { return sp->Config().name; };
    int ComX() { return comX; };
    int Bps() { return sp->Config().baudrate; };

    /*< ILayer --------------------------------------------------*/
    /// \brief  Called by upperLayer
    virtual bool IsTxReady() override;
    /// \brief  Called by upperLayer
    virtual int Tx(uint8_t * data, int len) override;

    /// \brief  Called by Eepoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /*--------------------------------------------------------->*/

private:
    uint8_t comX;
    SerialPort *sp;

    /// \brief  Called in EventsHandle
    int RxHandle();
};
