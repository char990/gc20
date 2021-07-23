#ifndef __TCPOPERATOR_H__
#define __TCPOPERATOR_H__

#include "IOperator.h"
#include "AppAdaptor.h"
#include "TimerEvent.h"
#include "BootTimer.h"


class TcpServer;

/// \brief  Operator from tcp
class TcpOperator : public IOperator, public IPeriodicEvent
{
public:
    TcpOperator();
    ~TcpOperator();
    static TimerEvent * tmrEvent;

    virtual void EventsHandle(uint32_t events) override;

    /// \brief  Called only after object was created
    virtual void Init(std::string name, std::string aType) override;

    /// \brief  Called when connection was accepted
    virtual void Setup(int fd) override;
    
    virtual int Tx(uint8_t * data, int len) override;

    virtual void PeriodicRun() override;

    void SetServer(TcpServer * server);
    void IdleTime(int idleTime);

    std::string Name(){ return name; };
protected:
    void Rx();

private:
    std::string name;
    AppAdaptor * adaptor;
    TcpServer * server;
    void Release();
    BootTimer tcpIdleTmr;
    int idleTime;
};

#endif
