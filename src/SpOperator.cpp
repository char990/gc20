#include <stdexcept>
#include "SpOperator.h"
#include "Epoll.h"
#include "TsiSp003Lower.h"
#include "Web2AppLower.h"

SpOperator::SpOperator(std::string name, int fd, ILowerLayer::LowerLayerType llType)
:name(name),
 llType(llType)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    switch (llType)
    {
    case ILowerLayer::LowerLayerType::TSISP003LOWER:
        lowerLayer = new TsiSp003Lower(name+"::TsiSp003Lower");
        break;
    case ILowerLayer::LowerLayerType::WEB2APPLOWER:
        throw std::runtime_error("SpOperator does not accept Web2AppLower");
        //lowerLayer = new Web2AppLower(name+"::Web2AppLower");
        break;
    }
    Epoll::Instance().AddEvent(this, events);
}

SpOperator::~SpOperator()
{    switch (llType)
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
        UnknownEvents(name,events);
    }
}

void SpOperator::Rx()
{

}

void SpOperator::Tx()
{

}
