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
#include <sign/Controller.h>

#include <uci.h>


const char * FirmwareMajorVer = "02";
const char * FirmwareMinorVer = "01";

using namespace std;


int main()
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    try
    {
        srand (time(NULL));
        Epoll::Instance().Init(64);
        TimerEvent timerEvt10ms(10,"[tmrEvt10ms:10ms]");
        TimerEvent timerEvt1s(1000,"[tmrEvt1sec:1sec]");
        DbHelper::Instance().Init();
        Controller::Instance().Init();

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

        TcpServer tcpServerPhcs{59991, ntsPool, &timerEvt1s};
        TcpServer tcpServerWeb{59992, webPool, &timerEvt1s};

        SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, 38400);
        SerialPort rs232(COMPORTS[0].device, spCfg);
        OprSp oprRs232(rs232, COMPORTS[0].name, "NTS");

        spCfg.mode = SerialPortConfig::SpMode::RS485_01;
        spCfg.baudrate = 115200;
        SerialPort com6(COMPORTS[6].device, spCfg);
        OprSp oprCom6(com6, COMPORTS[6].name, "NTS");


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
