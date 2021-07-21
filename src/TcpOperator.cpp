#include "TcpOperator.h"
#include "TsiSp003Lower.h"
#include "Web2AppLower.h"

TimerEvent * TcpOperator::tmrEvent = nullptr;

#define IDLE_TIME (60*1000)

TcpOperator::TcpOperator(std::string name, int fd, ILowerLayer::LowerLayerType llType)
    : name(name),
      llType(llType)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    switch (llType)
    {
    case ILowerLayer::LowerLayerType::TSISP003LOWER:
        lowerLayer = new TsiSp003Lower(name + "::TsiSp003Lower");
        break;
    case ILowerLayer::LowerLayerType::WEB2APPLOWER:
        lowerLayer = new Web2AppLower(name + "::Web2AppLower");
        break;
    }
    Epoll::Instance().AddEvent(this, events);
    tmrEvent->Add(this);
    tcpIdleTmr.Setms(IDLE_TIME);
}

TcpOperator::~TcpOperator()
{
    switch (llType)
    {
    case ILowerLayer::LowerLayerType::TSISP003LOWER:
        delete (TsiSp003Lower *)lowerLayer;
        break;
    case ILowerLayer::LowerLayerType::WEB2APPLOWER:
        delete (Web2AppLower *)lowerLayer;
        break;
    }
    Release();
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
        printf("[%s] disconnected\n", name.c_str());
        Release();
    }
    else
    {
        printf("[%s]%d bytes\n", name.c_str(), n);
    }
}

void TcpOperator::Tx()
{
    
}

void TcpOperator::EventsHandle(uint32_t events)
{
    if(events & (EPOLLRDHUP|EPOLLRDHUP|EPOLLERR))
    {
        printf("[%s%d] disconnected\n", name);
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
        UnknownEvents(name,events);
    }

}

void TcpOperator::PeriodicRun()
{
    if(tcpIdleTmr.IsExpired())
    {
        printf("[%s] idle. Disconnected\n", name.c_str());
        Release();
    }
}
