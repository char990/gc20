#include <stdexcept>

#include "Phcs2AppAdaptor.h"

Phcs2AppAdaptor::Phcs2AppAdaptor(std::string name, IOperator * iOperator)
:iOperator(iOperator)
{
    this->name = name+":PhcsAdaptor";
    sessionTimeout.Setms(-1);
    displayTimeout.Setms(-1);
    tmrEvent->Add(this);
}

Phcs2AppAdaptor::~Phcs2AppAdaptor()
{
    tmrEvent->Remove(this);
}

void Phcs2AppAdaptor::PeriodicRun()
{
    SessionTimeout();
}

void Phcs2AppAdaptor::SessionTimeout()
{
    if(sessionTimeout.IsExpired())
    {
        printf("[%s]Session timeout\n", name.c_str());
        sessionTimeout.Setms(-1);
    }
}

void Phcs2AppAdaptor::DisplayTimeout()
{
    if(displayTimeout.IsExpired())
    {
        printf("[%s]Display timeout\n", name.c_str());
        displayTimeout.Setms(-1);
    }
}

void Phcs2AppAdaptor::IncNr()
{
    nr++;
    if(nr==0)nr=1;
}

void Phcs2AppAdaptor::IncNs()
{
    ns++;
    if(ns==0)ns=1;
}

int Phcs2AppAdaptor::Rx(int fd)
{
    uint8_t buf[1024];
    int n = 0;
    while(1)
    {
        int k = read(fd,buf,1024);
        if(k<=0)break;
        n+=k;
    }
    sessionTimeout.Setms(30000);
    return n;
}

int Phcs2AppAdaptor::Tx(uint8_t * data, int len)
{
    return iOperator->Tx(data,len);
}
