#ifndef __EPOLL_H__
#define __EPOLL_H__

#include <sys/epoll.h>
#include "IGcEvent.h"

class Epoll
{
public:
    /// \brief		Constructor
    Epoll(int max);
    
    /// \brief		Destructor
    ~Epoll();

    /// \brief		Add Event
    void AddEvent(int fd, uint32_t state, IGcEvent * event);

    /// \brief		Delete Event
    void DeleteEvent(int fd, uint32_t state, IGcEvent * event);

    /// \brief		Modify Event
    void ModifyEvent(int fd, uint32_t state, IGcEvent * event);

    /// \brief      Event handle
    void EventHandle();

private:
    int epollfd;
    int cnt;
    const int MAX;
};

#endif
