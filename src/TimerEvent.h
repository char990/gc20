#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <time.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <stdexcept>
#include <string>
#include "IGcEvent.h"
#include "Epoll.h"

class TimerEvent : public IGcEvent
{
public:
    /// \brief		Constructor that sets up scheduler with period of ms.
    TimerEvent(int ms, std::string name, Epoll * epoll);
    
    /// \brief		Destructor. Closes fd.
    ~TimerEvent();

    /// \brief      Timer expired to run, refresh bootTime
    void InEvent();

    /// \brief      not used
    void OutEvent(){};

    /// \brief      Error event
    void Error(uint32_t events);

    /// \brief      file
    int SchedulerFd(){return schedulerFd;};

private:
    int schedulerFd;
    std::string name;
    int ticks, sec, cnt;
    Epoll *epoll;
};

#endif
