#ifndef __IGCEVENT_H__
#define __IGCEVENT_H__

#include <cstdio>
#include <string>
#include <stdexcept>
class IGcEvent
{
public:
    IGcEvent():events(0),eventFd(-1){};
    virtual ~IGcEvent(){};
    virtual void EventsHandle(uint32_t events)=0;

    int GetFd() { return eventFd; }

    virtual void UnknownEvents(std::string name, uint32_t events)
    {
        char buf[256];
        snprintf(buf,255,"[%s]Unkown events=0x%08X", name.c_str(), events);
        throw std::runtime_error(buf);
    }

    /// \brief  This is the events set to Epoll, could be used for DeleteEvent()
    uint32_t events;

    /// \brief  
    int eventFd;
};

#endif