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
#include <layer/StatusLed.h>



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

        // 2(tmr) + 1+3*2(nts) + 1+2*2(web) + 7*2(com) + 1(led) = 29
        Epoll::Instance().Init(32);
        TimerEvent timerEvt10ms(10,"[tmrEvt10ms:10ms]");
        TimerEvent timerEvt1s(1000,"[tmrEvt1sec:1sec]");
        DbHelper::Instance().Init();
        Scheduler::Instance().Init(&timerEvt10ms);
        StatusLed::Instance().Init(&timerEvt10ms);
        
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
            tcppool[i].Init("Tcp"+std::to_string(i), "NTS", (DbHelper::Instance().uciProd.SessionTimeout()+60)*1000);
        }

        TcpServer tcpServerPhcs{DbHelper::Instance().uciProd.SvcPort(), ntsPool, &timerEvt1s};
        TcpServer tcpServerWeb{DbHelper::Instance().uciProd.WebPort(), webPool, &timerEvt1s};
        

        // init serial ports
        SerialPort * sp[COMPORT_SIZE];
        for(int i=0;i<COMPORT_SIZE;i++)
        {
            sp[i]=nullptr;
        }

        OprSp * oprSp;
        { // tsi-sp-003 : serial port
            int cp = DbHelper::Instance().uciUser.ComPort();
            int bps = DbHelper::Instance().uciUser.Baudrate();
            if(cp==0)
            {
                SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, bps);
                sp[0] = new SerialPort(COMPORTS[0].device, spCfg);
                oprSp = new OprSp(*sp[0], COMPORTS[0].name, "NTS");
            }
            else
            {
                SerialPortConfig spCfg(SerialPortConfig::SpMode::RS485_01, bps);
                sp[cp] = new SerialPort(COMPORTS[cp].device, spCfg);
                oprSp = new OprSp(*sp[cp], COMPORTS[cp].name, "NTS");
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
