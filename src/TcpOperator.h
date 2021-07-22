#ifndef __TCPOPERATOR_H__
#define __TCPOPERATOR_H__

#include "IOperator.h"
#include "IAdaptLayer.h"
#include "TimerEvent.h"
#include "BootTimer.h"

/// \brief  Operator from tcp
class TcpOperator : public IOperator, public IPeriodicEvent
{
public:
    TcpOperator();
    ~TcpOperator();
    static TimerEvent * tmrEvent;

    virtual void EventsHandle(uint32_t events) override;

    /// \brief  Called only after object was created
    virtual void Init(IAdaptLayer::AdType adType, std::string name) override;

    /// \brief  Called when connection was accepted
    virtual void Setup(int fd) override;
    
    virtual int Tx(uint8_t * data, int len) override;

    void PeriodicRun();

protected:
    void Rx();

private:
    std::string name;
    IAdaptLayer::AdType adType;
    IAdaptLayer *lowerLayer;

    void Release();
    BootTimer tcpIdleTmr;
};

#endif
