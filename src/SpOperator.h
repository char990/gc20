#ifndef __SPOPERATOR_H__
#define __SPOPERATOR_H__

#include "IOperator.h"
#include "IAdaptLayer.h"
#include "SerialPort.h"

/// \brief  Operator from serial port
class SpOperator : public IOperator
{
public:
    SpOperator(std::string name, int fd, IAdaptLayer::AdType adType);
    ~SpOperator();
    void EventsHandle(uint32_t events);
    void Rx();
    void Tx();
    int GetFd() { return eventFd; };
    void Release();
private:
    std::string name;
    IAdaptLayer::AdType adType;
    IAdaptLayer *lowerLayer;
};

#endif
