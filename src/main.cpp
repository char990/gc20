/*
 *
 */
#include <cstdio>
#include <unistd.h>
#include "Epoll.h"
#include "SerialPort.h"
#include "TimerEvent.h"
#include "TsiSp003Lower.h"
#include "TsiSp003Cfg.h"
#include "DbHelper.h"
#include "TcpServer.h"
#include "TcpOperator.h"
#include "SpOperator.h"
#include "ObjectPool.h"

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
        #define LINKS_TSI   8   // from tcp:tsi-sp-003
        #define LINKS_WEB   2   // from web
        
        DbHelper::Instance().Init();
        Epoll::Instance().Init(64);

        TcpOperator * pool;
        ObjectPool<TcpOperator> webPool(LINKS_WEB);
        pool = webPool.Pool();
        for(int i=0;i<webPool.Size();i++)
        {
            pool[i].Init(IAdaptLayer::AdType::AT_W2A, "TcpOp:Web"+std::to_string(i));
        }

        ObjectPool<TcpOperator> tsiPool(LINKS_TSI);
        pool = tsiPool.Pool();
        for(int i=0;i<tsiPool.Size();i++)
        {
            pool[i].Init(IAdaptLayer::AdType::AT_TSI, "TcpOp:Tsi"+std::to_string(i));
        }

        

        TimerEvent timerEvt(10,"[timerEvt:10ms]");
        TsiSp003Lower::tmrEvent = &timerEvt;
        TcpOperator::tmrEvent = &timerEvt;

        TcpServer tcpServerTSiSp003(59991, tsiPool);
        TcpServer tcpServerWeb2App(59992, webPool);

        #define LINKS_SP    2   // from serial port
        SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, 115200);
        SerialPort rs232("/dev/ttyRS232", spCfg);
        rs232.Open();

        spCfg.mode = SerialPortConfig::SpMode::RS485_01;
        SerialPort com6("/dev/ttyCOM6", spCfg);
        com6.Open();

        /*************** Start ****************/
        while(1)
        {
            Epoll::Instance().EventsHandle();
        }
        /************* Never hit **************/
        com6.Close();
        rs232.Close();
    }
    catch(const std::exception& e)
    {
        printf("main exception: %s\n", e.what());
        // clean
    }
}


// E-O-F
