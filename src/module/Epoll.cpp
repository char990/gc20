#include <cstdio>
#include <unistd.h>
#include <module/MyDbg.h>
#include <module/Epoll.h>

void Epoll::Init(int max)
{
    if (max <= 0)
    {
        MyThrow("Epoll size must be greater than 0");
    }
    if (MAX > 0)
    {
        MyThrow("Epoll Re-Init is not allowed");
    }
    MAX = max;
    epollfd = epoll_create(MAX);
    if (epollfd < 0)
    {
        MyThrow("Epoll create failed");
    }
    events = new epoll_event[MAX];
}

Epoll::~Epoll()
{
    if (events != nullptr)
    {
        delete[] events;
    }
    if (epollfd > 0)
        close(epollfd);
}

void Epoll::AddEvent(IGcEvent *event, uint32_t events)
{
    if (evtSize >= MAX)
    {
        MyThrow("Epoll overflow. Can't add event.");
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, event->GetFd(), &ev);
    evtSize++;
}

void Epoll::DeleteEvent(IGcEvent *event, uint32_t events)
{
    if (evtSize == 0)
    {
        MyThrow("Epoll is empty. Can't delete event.");
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, event->GetFd(), &ev);
    evtSize--;
}

void Epoll::ModifyEvent(IGcEvent *event, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, event->GetFd(), &ev);
}

void Epoll::EventsHandle()
{
    int num = epoll_wait(epollfd, events, MAX, -1);
    if (num == -1)
    {
        if (errno != EINTR)
        {
            MyThrow("epoll_wait() error:%d", errno);
        }
    }
    for (int i = 0; i < num; i++)
    {
        IGcEvent *evt = (IGcEvent *)events[i].data.ptr;
        evt->EventsHandle(events[i].events);
    }
}
