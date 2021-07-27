/*
 *
 */
#include <cstdio>
#include <unistd.h>
#include <mcheck.h>

#include "Epoll.h"
#include "SerialPort.h"
#include "TimerEvent.h"
#include "TsiSp003Cfg.h"
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

int main()
{
    //mtrace();
    try
    {
        DbHelper::Instance().Init();
        Epoll::Instance().Init(64);
        TimerEvent timerEvt(10,"[tmrEvt:10ms]");
        IOperator::tmrEvent = &timerEvt;

        #define LINKS_PHCS   3   // from tcp:tsi-sp-003
        #define LINKS_WEB   2   // from web
        
        ObjectPool<OprTcp> webPool(LINKS_WEB);
        auto webpool = webPool.Pool();
        for(int i=0;i<webPool.Size();i++)
        {
            webpool[i].Init("Tcp:WEB"+std::to_string(i), "WEB");
            webpool[i].IdleTime(60*1000);
        }

        ObjectPool<OprTcp> phcsPool(LINKS_PHCS);
        auto tcppool = phcsPool.Pool();
        for(int i=0;i<phcsPool.Size();i++)
        {
            tcppool[i].Init("Tcp:PHCS"+std::to_string(i), "PHCS");
            tcppool[i].IdleTime(60*1000);
        }

        TcpServer tcpServerPhcs{59991, phcsPool};
        TcpServer tcpServerWeb{59992, webPool};

        SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, 115200);
        SerialPort rs232("/dev/ttyRS232", spCfg);
        OprSp oprRs232(&rs232);
        oprRs232.Init("RS232:PHCS", "PHCS");

        spCfg.mode = SerialPortConfig::SpMode::RS485_01;
        SerialPort com6("/dev/ttyCOM6", spCfg);
        OprSp oprCom6(&com6);
        oprCom6.Init("COM6:PHCS", "PHCS");

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
