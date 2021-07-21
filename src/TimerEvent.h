#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <time.h>
#include <sys/timerfd.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "Epoll.h"
#include "IPeriodicEvent.h"

class TimerEvent : public IGcEvent
{
public:
    /// \brief		Constructor that sets up scheduler with period of ms.
    TimerEvent(int ms, std::string name);
    
    /// \brief		Destructor. Closes fd.
    ~TimerEvent();

    /// \brief      Event handle, will be called in Epoll when events rise
    void EventsHandle(uint32_t events) override;

    /// \brief      Add delegate IPeriodicEvent
    void Add(IPeriodicEvent * evt);

    /// \brief      Add delegate IPeriodicEvent
    void Remove(IPeriodicEvent * evt);

    /// \brief      Periodic run
    void PeriodicRun();

private:
    std::string name;
    int ticks, sec, cnt;
    std::vector<IPeriodicEvent *> pEvts; 
};

#endif
