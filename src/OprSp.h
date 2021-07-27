#ifndef __SPOPERATOR_H__
#define __SPOPERATOR_H__

#include "SerialPort.h"
#include "IOperator.h"
#include "ILayer.h"

/// \brief  Operator from serial port
class OprSp : public IOperator, public ILayer
{
public:
    OprSp(SerialPort * sp);
    ~OprSp();

    /* ILayer */
    /// \brief  Called by upperLayer
    virtual int Tx(uint8_t * data, int len) override;
    /// \brief  Called by upperLayer
    virtual void Release() override;
    /// \brief  Called by IOperator
    virtual int Rx(uint8_t * data, int len) override;
    /// \brief  Called by IOperator
    virtual void PeriodicRun() override;
    /// \brief  Called by IOperator
    virtual void Clean() override;

    /* IOperator */
    /// \brief  Called by Eepoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /// \brief  Called when instantiation
    virtual void Init(std::string name, std::string aType) override;
    /// \brief  Called when serial port opened
    virtual void Setup(int fd) override;
    /// \brief  Called by ILayer.Tx()
    virtual int TxEvt(uint8_t * data, int len) override;
    /// \brief  Called by TimerEvt
    virtual void PeriodicEvt() override;

    /// \brief  Called in EventsHandle
    int RxHandle();

private:
    std::string name;
    SerialPort * sp;
};

#endif
