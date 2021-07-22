#include <stdexcept>
#include "SpOperator.h"
#include "Epoll.h"
#include "TsiSp003Lower.h"
#include "Web2AppLower.h"

SpOperator::SpOperator(std::string name, int fd, IAdaptLayer::AdType adType)
:name(name),
 adType(adType)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    switch (adType)
    {
    case IAdaptLayer::AdType::AT_TSI:
        lowerLayer = new TsiSp003Lower(name, this);
        break;
    case IAdaptLayer::AdType::AT_W2A:
        lowerLayer = new Web2AppLower(name, this);
        break;
    }
    Epoll::Instance().AddEvent(this, events);
}

SpOperator::~SpOperator()
{    switch (adType)
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

void SpOperator::Release()
{
    Epoll::Instance().DeleteEvent(this, events);
}

void SpOperator::EventsHandle(uint32_t events)
{
    if(events & (EPOLLRDHUP|EPOLLRDHUP|EPOLLERR))
    {
        printf("[%s] disconnected\n", name.c_str());
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

void SpOperator::Rx()
{

}

void SpOperator::Tx()
{

}
