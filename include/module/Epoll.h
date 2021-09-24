#pragma once


#include <sys/epoll.h>
#include <module/IGcEvent.h>

class Epoll
{
public:
    Epoll(Epoll const &) = delete;
    void operator=(Epoll const &) = delete;
    static Epoll &Instance()
    {
        static Epoll instance; // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    /// \brief		Init
    void Init(int max);

    /// \brief		Add Event
    void AddEvent(IGcEvent * event, uint32_t events);

    /// \brief		Delete Event
    void DeleteEvent(IGcEvent * event, uint32_t events);

    /// \brief		Modify Event
    void ModifyEvent(IGcEvent * event, uint32_t events);

    /// \brief      Event handle
    void EventsHandle();

private:
    Epoll():MAX(0),events(nullptr){};
    ~Epoll();
    int epollfd;
    int cnt;
    int MAX;
    epoll_event * events;
};
