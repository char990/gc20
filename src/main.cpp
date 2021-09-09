/*
 *
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

#include <layer/StatusLed.h>
#include <sign/Scheduler.h>


const char * FirmwareMajorVer = "01";
const char * FirmwareMinorVer = "50";

using namespace std;

void PrintBorder()
{
	#define BoerderSize (27+11+1+8)
	char buf[BoerderSize+1];
	memset(buf,'*',BoerderSize);
	buf[BoerderSize]='\0';
	printf ("%s\n",buf);
}

void PrintVersion()
{
	printf ("\n");
	PrintBorder();
	printf ("* Version %s.%s, Build at %s %s *\n",
        FirmwareMajorVer, FirmwareMinorVer, __DATE__, __TIME__ ); // 27 + 11 + 1 + 8
	PrintBorder();
}

int main()
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    PrintVersion();
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
            tcppool[i].Init("Tcp"+std::to_string(i), "NTS", (DbHelper::Instance().uciUser.SessionTimeout()+60)*1000);
        }

        TcpServer tcpServerPhcs{DbHelper::Instance().uciUser.SvcPort(), ntsPool, &timerEvt1s};
        TcpServer tcpServerWeb{DbHelper::Instance().uciUser.WebPort(), webPool, &timerEvt1s};
        
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
                sp[0] = new SerialPort(COM_DEV[0], spCfg);
                oprSp = new OprSp(*sp[0], COM_NAME[0], "NTS");
            }
            else
            {
                SerialPortConfig spCfg(SerialPortConfig::SpMode::RS485_01, bps);
                sp[cp] = new SerialPort(COM_DEV[cp], spCfg);
                oprSp = new OprSp(*sp[cp], COM_NAME[cp], "NTS");
            }

            for(int i=0;i<DbHelper::Instance().uciProd.NumberOfSigns();i++)
            {
                struct SignConnection * cn = DbHelper::Instance().uciProd.SignCn(i);
                if(cn->com_ip < COMPORT_SIZE)
                {// com port
                    if(sp[i]==nullptr)
                    {
                        SerialPortConfig spCfg(SerialPortConfig::SpMode::RS485_01, cn->bps_port);
                        sp[cn->com_ip] = new SerialPort(COM_DEV[cn->com_ip], spCfg);
                    }
                    else
                    {
                        if(cn->com_ip==cp)
                        {
                            MyThrow("Sign%d COM setting conflicts with UciUser.Comport", i+1);
                        }
                        if(sp[i]->Config().baudrate != cn->bps_port)
                        {
                            MyThrow("Sign%d baudrate error. All bps on %s should be same", i+1, COM_NAME[cn->com_ip]);
                        }
                    }
                }
                else
                {// ip

                }
            }
        }

        Scheduler::Instance().Init(&timerEvt10ms);
        StatusLed::Instance().Init(&timerEvt10ms);

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
