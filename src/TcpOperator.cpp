#include "TcpOperator.h"
#include "TsiSp003Lower.h"
#include "Web2AppLower.h"

TimerEvent * TcpOperator::tmrEvent = nullptr;

#define IDLE_TIME (60*1000)

TcpOperator::TcpOperator()
{
}

TcpOperator::~TcpOperator()
{
    switch (adType)
    {
    case IAdaptLayer::AdType::AT_TSI:
        delete (TsiSp003Lower *)lowerLayer;
        break;
    case IAdaptLayer::AdType::AT_W2A:
        delete (Web2AppLower *)lowerLayer;
        break;
    }
    Release();
}

void TcpOperator::Init(IAdaptLayer::AdType adType_, std::string name_)
{
    adType=adType_;
    name=name_;
    switch (adType)
    {
    case IAdaptLayer::AdType::AT_TSI:
        lowerLayer = new TsiSp003Lower(name+":TsiLower", this);
        break;
    case IAdaptLayer::AdType::AT_W2A:
        lowerLayer = new Web2AppLower(name+":WebLower", this);
        break;
    }
}

void TcpOperator::Setup(int fd)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    Epoll::Instance().AddEvent(this, events);
    tmrEvent->Add(this);
    tcpIdleTmr.Setms(IDLE_TIME);
}

void TcpOperator::Release()
{
    if (eventFd > 0)
    {
        tmrEvent->Remove(this);
        Epoll::Instance().DeleteEvent(this, events);
        close(eventFd);
        eventFd = -1;
    }
}

void TcpOperator::Rx()
{
    tcpIdleTmr.Setms(IDLE_TIME);
    int n = lowerLayer->Rx(eventFd);
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

int TcpOperator::Tx(uint8_t * data, int len)
{
    
}

void TcpOperator::EventsHandle(uint32_t events)
{
    if(events & (EPOLLRDHUP|EPOLLRDHUP|EPOLLERR))
    {
        printf("Disconnected:events=0x%08X\n",events);
        Release();
    }
    else if(events & EPOLLIN)
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
    if(tcpIdleTmr.IsExpired())
    {
        printf("Idle timeout. Disconnected\n");
        Release();
    }
}
