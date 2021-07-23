#include <cstdint>
#include <cstdio>

#include <unistd.h>

#include "TimerEvent.h"

TimerEvent::TimerEvent(int ms, std::string name):name(name)
{
    int tv_sec=ms/1000;
    long tv_nsec=(ms%1000)*1000*1000;

    struct itimerspec new_value;
    new_value.it_value.tv_sec  = tv_sec;
    new_value.it_value.tv_nsec = tv_nsec;
    new_value.it_interval.tv_sec = tv_sec;
    new_value.it_interval.tv_nsec = tv_nsec;

    eventFd = timerfd_create(CLOCK_BOOTTIME, TFD_NONBLOCK);
    if(eventFd == -1)
    {
		throw std::runtime_error("timerfd_create() failed");
    }

    if(timerfd_settime(eventFd, 0, &new_value, NULL) == -1)
    {
		throw std::runtime_error("timerfd_settime() failed");
    }
    ticks = 1000 / ms;
    sec=0;
    cnt=0;
    events = EPOLLIN | EPOLLET;
    Epoll::Instance().AddEvent(this, events);
}

TimerEvent::~TimerEvent()
{
    Epoll::Instance().DeleteEvent(this, events);
    if(eventFd>0)close(eventFd);
}

void TimerEvent::EventsHandle(uint32_t events)
{
    if(events & EPOLLIN)
    {
        PeriodicRun();
    }
    else
    {
        UnknownEvents(name,events);
    }
}

void TimerEvent::PeriodicRun()
{
    uint64_t buf;
    int r = read(eventFd,&buf,sizeof(uint64_t));
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
    for(int i=0;i<pEvts.size();i++)
    {
        if(pEvts[i]!=nullptr)
        {
            pEvts[i]->PeriodicRun();
        }
    }
}

void TimerEvent::Add(IPeriodicEvent * evt)
{
    for(int i=0;i<pEvts.size();i++)
    {
        if(pEvts[i]==nullptr)
        {
            pEvts[i]=evt;
            return;
        }
    }
    pEvts.push_back(evt);
}

void TimerEvent::Remove(IPeriodicEvent * evt)
{
    for(int i=0;i<pEvts.size();i++)
    {
        if(pEvts[i]==evt)
        {
            pEvts[i]=nullptr;
            return;
        }
    }
}