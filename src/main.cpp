/*
 *
 */
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <mcheck.h>

#include <module/Epoll.h>
#include <module/SerialPort.h>
#include <module/TimerEvent.h>
#include <uci/DbHelper.h>
#include <module/TcpServer.h>
#include <module/OprTcp.h>
#include <module/OprSp.h>
#include <module/ObjectPool.h>
#include <sign/Scheduler.h>

#include <uci.h>


const char * FirmwareMajorVer = "01";
const char * FirmwareMinorVer = "50";

using namespace std;


int main()
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    try
    {
        srand (time(NULL));
        #define LINKS_NTS   3   // from tcp-tsi-sp-003-nts
        #define LINKS_WEB   2   // from web

        // 2(tmr) + 1+3*2(nts) + 1+2*2(web) + 7*2(com) = 28
        Epoll::Instance().Init(32);
        TimerEvent timerEvt10ms(10,"[tmrEvt10ms:10ms]");
        TimerEvent timerEvt1s(1000,"[tmrEvt1sec:1sec]");
        DbHelper::Instance().Init();
        Scheduler::Instance().Init(&timerEvt10ms);
        
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

        TcpServer tcpServerPhcs{DbHelper::Instance().uciProd.SvcPort(), ntsPool, &timerEvt1s};
        TcpServer tcpServerWeb{DbHelper::Instance().uciProd.WebPort(), webPool, &timerEvt1s};
        
        SerialPort * sp[COMPORT_SIZE];
        for(int i=0;i<COMPORT_SIZE;i++)
        {
            sp[i]=nullptr;
        }

        uint8_t cp = DbHelper::Instance().uciUser.ComPort();
        int bps = DbHelper::Instance().uciUser.Baudrate();
        SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, bps);
        if(cp==0)
        {
            SerialPort rs232(COMPORTS[0].device, spCfg);
            OprSp oprRs232(rs232, COMPORTS[0].name, "NTS");
        }
        else
        {

        }

        for(int i=1;i<COMPORT_SIZE;i++)
        {
            if(COMPORTS[i].bps != 0)
        }        
        // ComDev COMPORTS[COMPORT_SIZE]
        if(COMPORTS[0].bps != 0)
        {
            SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, COMPORTS[0].bps);
            SerialPort rs232(COMPORTS[0].device, spCfg);
            OprSp oprRs232(rs232, COMPORTS[0].name, "NTS");
        }

        for(int i=1;i<COMPORT_SIZE;i++)
        {
            if(COMPORTS[i].bps != 0)
            {
                SerialPortConfig spCfg(SerialPortConfig::SpMode::RS485_01, COMPORTS[i].bps);
                SerialPort rs232(COMPORTS[i].device, spCfg);
                OprSp oprRs232(rs232, COMPORTS[i].name, "NTS");
            }
        }

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
