#ifndef __SPOPERATOR_H__
#define __SPOPERATOR_H__

#include <module/SerialPort.h>
#include <module/IOperator.h>
#include <layer/ILayer.h>

/// \brief  Operator from serial port
class OprSp : public IOperator
{
public:
    OprSp(SerialPort & sp, std::string name_, std::string aType);
    ~OprSp();

    /*< ILayer --------------------------------------------------*/
    /// \brief  Called by upperLayer
    virtual bool IsTxReady() override;
    /// \brief  Called by upperLayer
    virtual int Tx(uint8_t * data, int len) override;

    /// \brief  Called by Eepoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /*--------------------------------------------------------->*/

private:
    std::string name;
    SerialPort & sp;

    /// \brief  Called in EventsHandle
    int RxHandle();
};

#endif
