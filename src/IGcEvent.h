#ifndef __IGCEVENT_H__
#define __IGCEVENT_H__

#include <sys/epoll.h>

class IGcEvent
{
public:
    virtual void InEvent()=0;
    virtual void OutEvent()=0;
    virtual void Error(uint32_t events)=0;
};

#endif
