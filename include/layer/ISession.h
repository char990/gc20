#ifndef __ITSISP003SESSION_H__
#define __ITSISP003SESSION_H__

#include <cstdint>

class ISession
{
public:
    enum SESSION {OFF_LINE, START, ON_LINE};
    virtual enum SESSION Session()=0;
    virtual void Session(enum SESSION v)=0;
    virtual uint8_t Seed()=0;
    virtual void Seed(uint8_t v)=0;
};

#endif