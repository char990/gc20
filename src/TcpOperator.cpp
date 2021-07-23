#include "TcpOperator.h"
#include "TcpServer.h"
#include "Phcs2AppAdaptor.h"
#include "Web2AppAdaptor.h"

TimerEvent *TcpOperator::tmrEvent = nullptr;

TcpOperator::TcpOperator()
{
}

TcpOperator::~TcpOperator()
{
    if (adaptor)
    {
        delete adaptor;
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

void TcpOperator::Init(std::string name, std::string aType)
{
    this->name = name;
    adaptor = new AppAdaptor(name, aType, this);
}

void TcpOperator::IdleTime(int idleTime)
{
    this->idleTime = idleTime;
}

void TcpOperator::Setup(int fd)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    Epoll::Instance().AddEvent(this, events);
    tmrEvent->Add(this);
    tcpIdleTmr.Setms(idleTime);
}

void TcpOperator::SetServer(TcpServer *svr)
{
    server = svr;
}

void TcpOperator::Release()
{
    if (eventFd > 0)
    {
        tmrEvent->Remove(this);
        Epoll::Instance().DeleteEvent(this, events);
        events = 0;
        server->Release(this);
    }
}

void TcpOperator::Rx()
{
    tcpIdleTmr.Setms(idleTime);
    int n = adaptor->Rx(eventFd);
    if (n <= 0)
    {
        printf("disconnected\n");
        Release();
    }
    else
    {
        printf("%d bytes\n", n);
    }
}

int TcpOperator::Tx(uint8_t *data, int len)
{
}

void TcpOperator::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        printf("Disconnected:events=0x%08X\n", events);
        Release();
    }
    else if (events & EPOLLIN)
    {
        Rx();
    }
    else if (events & EPOLLOUT)
    {
    }
    else
    {
        UnknownEvents(name, events);
    }
}

void TcpOperator::PeriodicRun()
{
    if (tcpIdleTmr.IsExpired())
    {
        printf("Idle timeout. Disconnected\n");
        Release();
    }
}
