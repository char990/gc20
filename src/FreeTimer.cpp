#include <limits.h>

#include "FreeTimer.h"

void FreeTimer::SetMs(int ms)
{
    if(ms<0)
    {
        ns=0;
        sec=LONG_MAX;   // time_t is long
        return;
    }
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    ns = (ms%1000)*1000000 + _CLOCK_BOOTTIME.tv_nsec;
    sec = ms/1000 + _CLOCK_BOOTTIME.tv_sec;
    if(ns>1000000000)
    {
        ns-=1000000000;
        sec++;
    }
}

bool FreeTimer::IsExpired()
{
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    return ((_CLOCK_BOOTTIME.tv_sec>sec) ||
             (_CLOCK_BOOTTIME.tv_sec==sec && _CLOCK_BOOTTIME.tv_nsec>=ns));
}
