#pragma once

#include <cstdint>

class ISession
{
public:
    virtual ~ISession(){};
    enum SESSION {OFF_LINE, START, ON_LINE};
    virtual enum SESSION Session()=0;
    virtual void Session(enum SESSION v)=0;
    virtual uint8_t Seed()=0;
    virtual void Seed(uint8_t v)=0;
};

