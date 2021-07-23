#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include "Web2AppAdaptor.h"

Web2AppAdaptor::Web2AppAdaptor(std::string name, IOperator * iOperator)
:iOperator(iOperator)
{
    this->name = name+":WebAdaptor";
}

Web2AppAdaptor::~Web2AppAdaptor()
{
}

void Web2AppAdaptor::PeriodicRun()
{

}

int Web2AppAdaptor::Rx(int fd)
{
    uint8_t buf[65536];
    int n = 0;
    while(1)
    {
        int k = read(fd,buf,65536);
        if(k<=0)break;
        n+=k;
    }
    return n;
}

int Web2AppAdaptor::Tx(uint8_t * data, int len)
{
    return iOperator->Tx(data, len);
}
