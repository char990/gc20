#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include "Web2AppLower.h"

Web2AppLower::Web2AppLower(std::string name)
:name(name)
{

}

void Web2AppLower::PeriodicRun()
{

}

int Web2AppLower::Rx(int fd)
{
    uint8_t buf[65536];
    int n = 0;
    while(1)
    {
        int k = read(fd,buf,65536);
        if(k<0)break;
        n+=k;
    }
    return n;
}

int Web2AppLower::Tx(uint8_t * data, int len)
{
    return operator->Tx(data, len);
}
