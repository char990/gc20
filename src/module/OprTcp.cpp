#include <cstring>
#include <module/OprTcp.h>
#include <module/TcpServer.h>
#include <module/Epoll.h>
#include <layer/UI_LayerManager.h>
#include <uci/DbHelper.h>

#define TCPSPEED 1000000 // 1M bytes per seconds

OprTcp::OprTcp()
{
}

OprTcp::~OprTcp()
{
    if (upperLayer!=nullptr)
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
        PrintDbg(DBG_PRT, "%s-%s:Disconnected:events=0x%08X", name.c_str(), client, events);
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

long OprTcp::IdleMs()
{
    long s = DbHelper::Instance().GetUciUser().SessionTimeout();
    long t = DbHelper::Instance().GetUciProd().TcpTimeout();
    return (s + t)*1000;
}

/// \brief  Called when instantiation
void OprTcp::Init(std::string name_, std::string aType)
{
    name = name_;
    upperLayer = new UI_LayerManager(name, aType);
    upperLayer->LowerLayer(this);
}

/// \brief  Called when a new connection accepted
void OprTcp::Accept(int fd, TimerEvent *tmr, const char *client)
{
    strncpy(this->client, client, 23);
    eventFd = fd;
    events = EPOLLIN | EPOLLRDHUP;
    Epoll::Instance().AddEvent(this, events);
    tmrEvt = tmr;
    tmrEvt->Add(this);
    tcpIdleTmr.Setms(IdleMs());
    upperLayer->ClrRx();
    ClrTx();
}

/// \brief  Called by upperLayer->Tx()
int OprTcp::Tx(uint8_t *data, int len)
{
    int x = TxBytes(data, len);
    if (x > 0)
    {
        x = len * 1000 / TCPSPEED; // get ms
    }
    else if (x < 0)
    {
        PrintDbg(DBG_LOG, "%s-%s:Tx failed", name.c_str(), client);
        Release();
    }
    return (x < 10 ? 10 : x);
}

/// \brief  Called by TimerEvt
void OprTcp::PeriodicRun()
{
    if (tcpIdleTmr.IsExpired())
    {
        PrintDbg(DBG_PRT, "%s-%s:Idle timeout. Disconnected", name.c_str(), client);
        Release();
    }
}

/// \brief  Called in EventsHandle
int OprTcp::RxHandle()
{
#define buf_PAGE_SIZE 4096
    uint8_t buf[buf_PAGE_SIZE];
    tcpIdleTmr.Setms(IdleMs());
    while (1)
    {
        int n = read(eventFd, buf, buf_PAGE_SIZE);
        if (n <= 0)
        { // no data
            return n;
        }
        else
        {
            if (IsTxRdy())
            {
                upperLayer->Rx(buf, n);
            }
            else
            { // if tx is busy, discard this rx
                PrintDbg(DBG_PRT, "%s-%s:Tx not ready, discard rx", name.c_str(), client);
            }
        }
    }
}

/// \brief
void OprTcp::Release()
{
    tcpIdleTmr.Clear();
    if (tmrEvt != nullptr)
    {
        tmrEvt->Remove(this);
        tmrEvt = nullptr;
    }
    if (events > 0)
    {
        Epoll::Instance().DeleteEvent(this, events);
        events = 0;
    }
    if (eventFd > 0)
    {
        close(eventFd);
        eventFd = -1;
        ParentPool()->Push(this);
    }
}

void OprTcp::PopClean()
{
}

void OprTcp::PushClean()
{
}
