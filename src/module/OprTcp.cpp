#include <cstring>
#include <module/OprTcp.h>
#include <module/TcpServer.h>
#include <module/Epoll.h>
#include <layer/UI_LayerManager.h>

#define TCPSPEED 1000000    // 1M bytes per seconds

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
    upperLayer = new UI_LayerManager(name, aType);
    upperLayer->LowerLayer(this);
    idleTime = idle;
}

/// \brief  Called when a new connection accepted
void OprTcp::Setup(int fd, TimerEvent *tmr)
{
    upperLayer->ClrRx();
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    Epoll::Instance().AddEvent(this, events);
    tmrEvt = tmr;
    tmrEvt->Add(this);
    tcpIdleTmr.Setms(idleTime);
}

/// \brief  Called by upperLayer->Tx()
int OprTcp::Tx(uint8_t *data, int len)
{
    int x = TxBytes(data, len);
    if (x > 0)
    {
        x = len*1000 / TCPSPEED;    // get ms
    }
    return (x < 10 ? 10 : x);
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
    while (1)
    {
        int n = read(eventFd, buf, 4096);
        if (n <= 0)
        {
            return n;
        }
        else
        {
            printf("TcpRx %d bytes\n", n);
            if (IsTxRdy()) // if tx is busy, discard this rx
            {
                upperLayer->Rx(buf, n);
            }
            else
            {
                printf("TcpTx not ready\n");
            }
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
    if(tmrEvt!=nullptr)
    {
        tmrEvt->Remove(this);
    }
    if (events > 0)
    {
        Epoll::Instance().DeleteEvent(this, events);
        events = 0;
    }
    if(server!=nullptr)
    {
        server->Release(this);
    }
    eventFd = -1;
}
