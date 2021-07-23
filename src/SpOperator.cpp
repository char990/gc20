#include <stdexcept>
#include "SpOperator.h"
#include "Epoll.h"

SpOperator::SpOperator(std::string name, int fd, std::string adType)
:name(name)
{
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = fd;
    adaptor = new AppAdaptor(name, adType, this);
    Epoll::Instance().AddEvent(this, events);
}

SpOperator::~SpOperator()
{
    delete adaptor;
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
