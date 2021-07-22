#include <stdexcept>

#include "TsiSp003Lower.h"

TimerEvent * TsiSp003Lower::tmrEvent = nullptr;

TsiSp003Lower::TsiSp003Lower(std::string name, IOperator * iOperator)
:name(name),
 iOperator(iOperator)
{
    sessionTimeout.Setms(-1);
    displayTimeout.Setms(-1);
    tmrEvent->Add(this);
}

TsiSp003Lower::~TsiSp003Lower()
{
    tmrEvent->Remove(this);
}

void TsiSp003Lower::PeriodicRun()
{
    SessionTimeout();
}

void TsiSp003Lower::SessionTimeout()
{
    if(sessionTimeout.IsExpired())
    {
        printf("[%s]Session timeout\n", name.c_str());
        sessionTimeout.Setms(-1);
    }
}

void TsiSp003Lower::DisplayTimeout()
{
    if(displayTimeout.IsExpired())
    {
        printf("[%s]Display timeout\n", name.c_str());
        displayTimeout.Setms(-1);
    }
}

void TsiSp003Lower::IncNr()
{
    nr++;
    if(nr==0)nr=1;
}

void TsiSp003Lower::IncNs()
{
    ns++;
    if(ns==0)ns=1;
}

int TsiSp003Lower::Rx(int fd)
{
    uint8_t buf[65536];
    int n = 0;
    while(1)
    {
        int k = read(fd,buf,65536);
        if(k<0)break;
        n+=k;
    }
    sessionTimeout.Setms(30000);
    return n;
}

int TsiSp003Lower::Tx(uint8_t * data, int len)
{
    return 1;
}
