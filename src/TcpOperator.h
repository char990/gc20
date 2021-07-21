#ifndef __TCPOPERATOR_H__
#define __TCPOPERATOR_H__

#include "IOperator.h"
#include "ILowerLayer.h"
#include "TimerEvent.h"
#include "BootTimer.h"

/// \brief  Operator from tcp
class TcpOperator : public IOperator, public IPeriodicEvent
{
public:
    TcpOperator(std::string name, int fd, ILowerLayer::LowerLayerType llType);
    ~TcpOperator();
    static TimerEvent * tmrEvent;
    void EventsHandle(uint32_t events);
    void PeriodicRun();

protected:
    void Rx();
    void Tx();
    ILowerLayer *lowerLayer;

private:
    std::string name;
    ILowerLayer::LowerLayerType llType;
    void Release();
    BootTimer tcpIdleTmr;
};

#endif
