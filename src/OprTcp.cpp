#include <cstring>
#include "OprTcp.h"
#include "TcpServer.h"
#include "LayerAdaptor.h"
#include "TimerEvent.h"

#define TCPSPEED 1000

OprTcp::OprTcp()
{
}

OprTcp::~OprTcp()
{
    if (upperLayer)
    {
        delete upperLayer;
    }
    if (events > 0)
    {
        tmrEvent->Remove(this);
        Epoll::Instance().DeleteEvent(this, events);
    }
    if (eventFd > 0)
    {
        close(eventFd);
        eventFd = -1;
    }
}

/* ILayer */
/// \brief  Called by upperLayer
int OprTcp::Tx(uint8_t *data, int len)
{
    return TxEvt(data, len);
}

/// \brief  Called by upperLayer
void OprTcp::Release()
{
    if (eventFd > 0)
    {
        tmrEvent->Remove(this);
        Epoll::Instance().DeleteEvent(this, events);
        events = 0;
        server->Release(this);
    }
}

/// \brief  Called by IOperator
int OprTcp::Rx(uint8_t * data, int len)
{
    tcpIdleTmr.Setms(idleTime);
    return upperLayer->Rx(data, len);
}

/// \brief  Called by IOperator
void OprTcp::PeriodicRun()
{
    if (tcpIdleTmr.IsExpired())
    {
        printf("Idle timeout. Disconnected\n");
        Release();
    }
    else
    {
        upperLayer->PeriodicRun();
    }
}

/// \brief  Called by IOperator
void OprTcp::Clean()
{
    upperLayer->Clean();
}

/* IOperator */
/// \brief  Called by Eepoll, receiving & sending handle
void OprTcp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        printf("Disconnected:events=0x%08X\n", events);
        Release();
    }
    else if (events & EPOLLIN)
    {
        RxHandle();
    }
    else if (events & EPOLLOUT)
    {
        TxHandle();
    }
    else
    {
        UnknownEvents(name, events);
    }
}

/// \brief  Called when instantiation
void OprTcp::Init(std::string name_, std::string aType)
{
    name = name_ + ":OprTcp";
    upperLayer = new LayerAdaptor(name, aType, this);
}

/// \brief  Called when a new connection accepted
void OprTcp::Setup(int fd)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    Epoll::Instance().AddEvent(this, events);
    tmrEvent->Add(this);
    tcpIdleTmr.Setms(idleTime);
    Clean();
}

/// \brief  Called by ILayer.Tx()
int OprTcp::TxEvt(uint8_t * data, int len)
{
    int x = TxBytes(data, len);
    if(x>0)
    {
        x = len/TCPSPEED;
    }
    return (x==0 ? 1 : x);
}

/// \brief  Called by TimerEvt
void OprTcp::PeriodicEvt()
{
    PeriodicRun();
}

/// \brief  Called in EventsHandle
int OprTcp::RxHandle()
{
    uint8_t buf[4096];
    int n = read(eventFd, buf, 4096);
    if (n <= 0)
    {
        printf("disconnected\n");
        Release();
    }
    else
    {
        Rx(buf, n);
        //printf("%d bytes\n", n);
    }
    return n;
}

/// --------------------------------------
void OprTcp::SetServer(TcpServer *svr)
{
    server = svr;
}

void OprTcp::IdleTime(int idleTime)
{
    this->idleTime = idleTime;
}

