/*
 *
 */
#include <cstdio>
#include <unistd.h>
#include "Epoll.h"
#include "SerialPort.h"
#include "TimerEvent.h"
#include "TsiSp003.h"

using namespace std;

void PrintTime()
{
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    printf("[%ld.%09ld]",_CLOCK_BOOTTIME.tv_sec,_CLOCK_BOOTTIME.tv_nsec);
}

long Interval()
{
    struct timespec start={0,0};
    struct timespec end;
    clock_gettime(CLOCK_BOOTTIME, &end);
    long ms = (end.tv_sec - start.tv_sec)*1000;
    if(end.tv_nsec < start.tv_nsec)
    {
        ms += (end.tv_nsec + 1000000000 - start.tv_nsec)/1000000 - 1000;
    }
    else
    {
        ms += (end.tv_nsec - start.tv_nsec)/1000000;
    }
    start=end;
    return ms;
}

int main()
{
    try
    {
        for(int i=0;i<MAX_TsiSp003;i++)
        {
            TsiSp003::tsiSp003s[i]=nullptr;
        }
        
        Epoll epoll(32);
        TimerEvent timerEvt(10,"[timerEvt:10ms]", &epoll);

        SerialPortConfig cfg(SerialPortConfig::SpMode::RS232, 115200);
        SerialPort rs232("/dev/ttyRS232", cfg, &epoll);
        IByteStream *irs232 = &rs232;

        irs232->Open();

        while(1)
        {
            epoll.EventHandle();
        }
        irs232->Close();
    }
    catch(const std::exception& e)
    {
        printf("main exception: %s\n", e.what());
    }
}


// E-O-F
