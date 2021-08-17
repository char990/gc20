#include <cstring>
#include <module/OprTcp.h>
#include <module/TcpServer.h>
#include <module/Epoll.h>
#include <layer/LayerManager.h>

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
    Release();
}

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
void OprTcp::Init(std::string name_, std::string aType, int idle)
{
    name = name_;
    upperLayer = new LayerManager(name, aType);
    upperLayer->LowerLayer(this);
    idleTime = idle;
}

/// \brief  Called when a new connection accepted
void OprTcp::Setup(int fd,TimerEvent * tmr)
{
    upperLayer->Clean();
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    Epoll::Instance().AddEvent(this, events);
    tmrEvt=tmr;
    tmrEvt->Add(this);
    tcpIdleTmr.Setms(idleTime);
}

/// \brief  Called by upperLayer->Tx()
int OprTcp::Tx(uint8_t * data, int len)
{
    int x = TxBytes(data, len);
    if(x>0)
    {
        x = len/TCPSPEED;
    }
    return (x==0 ? 1 : x);
}

/// \brief  Called by TimerEvt
void OprTcp::PeriodicRun()
{
    if (tcpIdleTmr.IsExpired())
    {
        printf("Idle timeout. Disconnected\n");
        Release();
    }
}

/// \brief  Called in EventsHandle
int OprTcp::RxHandle()
{
    uint8_t buf[4096];
    tcpIdleTmr.Setms(idleTime);
    while(1)
    {
        int n = read(eventFd, buf, 4096);
        if(n<=0)
        {
            return n;
        }
        else
        {
            printf("%d bytes\n", n);
            upperLayer->Rx(buf, n);
        }
    }
}

/// --------------------------------------
void OprTcp::SetServer(TcpServer *svr)
{
    server = svr;
}

/// \brief  
void OprTcp::Release()
{
    tcpIdleTmr.Clear();
    tmrEvt->Remove(this);
    if (events > 0)
    {
        Epoll::Instance().DeleteEvent(this, events);
        events = 0;
    }
    server->Release(this);
    eventFd = -1;
}
