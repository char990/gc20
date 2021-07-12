#include <cstdint>
#include <cstdio>

#include <unistd.h>

#include "TimerEvent.h"

TimerEvent::TimerEvent(int ms, std::string name, Epoll* epoll):name(name),epoll(epoll)
{
    int tv_sec=ms/1000;
    long tv_nsec=(ms%1000)*1000*1000;

    struct itimerspec new_value;
    new_value.it_value.tv_sec  = tv_sec;
    new_value.it_value.tv_nsec = tv_nsec;
    new_value.it_interval.tv_sec = tv_sec;
    new_value.it_interval.tv_nsec = tv_nsec;

    schedulerFd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    if(schedulerFd == -1)
    {
		throw std::runtime_error("timerfd_create() failed");
    }

    if(timerfd_settime(schedulerFd, 0, &new_value, NULL) == -1)
    {
		throw std::runtime_error("timerfd_settime() failed");
    }
    ticks = 1000 / ms;
    sec=0;
    cnt=0;
    epoll->AddEvent(schedulerFd, EPOLLIN | EPOLLET, this);
}

TimerEvent::~TimerEvent()
{
    if(epoll>0)epoll->DeleteEvent(schedulerFd, EPOLLIN | EPOLLET, this);
    if(schedulerFd>0)close(schedulerFd);
}

void TimerEvent::Error(uint32_t events)
{
    if (events & EPOLLERR)
    {
        throw std::runtime_error(name + "InEvent failed:EPOLLERR");
    }
    if (events & EPOLLHUP)
    {
        throw std::runtime_error(name + "InEvent failed:EPOLLHUP");
    }
    if (events & EPOLLRDHUP)
    {
        throw std::runtime_error(name + "InEvent failed:EPOLLRDHUP");
    }
}

void TimerEvent::InEvent()
{
    uint64_t buf;
    int r = read(schedulerFd,&buf,sizeof(uint64_t));
    if(r<0)
    {
        throw std::runtime_error(name + "InEvent failed:read");
    }
    if(++cnt==ticks)
    {
        cnt=0;
        sec++;
        printf("(%s)sec=%d\n", name.c_str(), sec);
    }
}
