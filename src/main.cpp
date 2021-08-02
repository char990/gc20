/*
 *
 */
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <mcheck.h>

#include "Epoll.h"
#include "SerialPort.h"
#include "TimerEvent.h"
#include "DbHelper.h"
#include "DbHelper.h"
#include "TcpServer.h"
#include "OprTcp.h"
#include "OprSp.h"
#include "ObjectPool.h"

using namespace std;


void PrintTime()
{
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    printf("[%ld.%09ld]\n",_CLOCK_BOOTTIME.tv_sec,_CLOCK_BOOTTIME.tv_nsec);
}

long Interval()
{
    static struct timespec start={0,0};
    static struct timespec end;
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

TimerEvent * tmrEvt;
int main()
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    try
    {
        srand (time(NULL));
        DbHelper::Instance().Init();
        Epoll::Instance().Init(64);
        TimerEvent timerEvt(10,"[tmrEvt:10ms]");
        tmrEvt = &timerEvt;

        #define LINKS_NTS   3   // from tcp:tsi-sp-003
        #define LINKS_WEB   2   // from web
        
        ObjectPool<OprTcp> webPool(LINKS_WEB);
        auto webpool = webPool.Pool();
        for(int i=0;i<webPool.Size();i++)
        {
            webpool[i].Init("Tcp"+std::to_string(i), "WEB", 60*1000);
        }

        ObjectPool<OprTcp> ntsPool(LINKS_NTS);
        auto tcppool = ntsPool.Pool();
        for(int i=0;i<ntsPool.Size();i++)
        {
            tcppool[i].Init("Tcp"+std::to_string(i), "NTS", 60*1000);
        }

        TcpServer tcpServerPhcs{59991, ntsPool};
        TcpServer tcpServerWeb{59992, webPool};

        SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, 38400);
        SerialPort rs232("/dev/ttyRS232", spCfg);
        OprSp oprRs232(rs232, "RS232", "NTS");

        spCfg.mode = SerialPortConfig::SpMode::RS485_01;
        spCfg.baudrate = 115200;
        SerialPort com6("/dev/ttyCOM6", spCfg);
        OprSp oprCom6(com6, "COM6", "NTS");


        /*************** Start ****************/
        while(1)
        {
            Epoll::Instance().EventsHandle();
        }
        /************* Never hit **************/
    }
    catch(const std::exception& e)
    {
        printf("main exception: %s\n", e.what());
        // clean
    }
    //muntrace();
}

// E-O-F
