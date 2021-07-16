#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <time.h>
#include <sys/timerfd.h>
#include <stdexcept>
#include <string>
#include <list>

#include "Epoll.h"
#include "IPeriodicEvent.h"

class TimerEvent : public IGcEvent
{
public:
    /// \brief		Constructor that sets up scheduler with period of ms.
    TimerEvent(int ms, std::string name);
    
    /// \brief		Destructor. Closes fd.
    ~TimerEvent();

    /// \brief      Timer expired to run
    void InEvent() override;

    /// \brief      not used
    void OutEvent() override {};

    /// \brief      Error event
    void Error(uint32_t events) override;

    /// \brief      file
    int GetFd() override {return schedulerFd; };

    /// \brief      Add delegate IPeriodicEvent
    void Add(IPeriodicEvent * evt);

    /// \brief      Add delegate IPeriodicEvent
    void Remove(IPeriodicEvent * evt);

private:
    int schedulerFd;
    std::string name;
    int ticks, sec, cnt;
    std::list<IPeriodicEvent *> evts; 
};

#endif
