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

const char *FirmwareMajorVer = "01";
const char *FirmwareMinorVer = "50";

using namespace std;

void PrintBorder()
{
#define BoerderSize (16 + 10 + 11 + 1 + 8 + 2)
    char buf[BoerderSize + 1];
    memset(buf, '*', BoerderSize);
    buf[BoerderSize] = '\0';
    printf("%s\n", buf);
}

void PrintVersion()
{
    printf("\n");
    PrintBorder();
    printf("* Version %s.%s, Build at %s %s *\n",
           FirmwareMajorVer, FirmwareMinorVer, __DATE__, __TIME__); // 27 + 11 + 1 + 8
    PrintBorder();
}

class TickTock : public IPeriodicRun
{
public:
    TickTock():sec(0),min(0),hour(0),day(0){};
    virtual void PeriodicRun() override
    {
        if(++sec==60)
        {
            sec=0;
            if(++min==60)
            {
                min=0;
                if(++hour==24)
                {
                    hour=0;
                    day++;
                }
            }
        }
        printf("Day(%d) %d:%02d:%02d\n",day,hour,min,sec);
    };
private:
    int sec;
    int min;
    int hour;
    int day;
};

void crc8005()
{
    uint8_t buf1[]={0x02,0x30,0x31,0x30,0x35};
    printf("\ncrc1(18F3):%04X\n", Utils::Crc::Crc16_8005(buf1,sizeof(buf1)));
    uint8_t buf2[]={
        0x02,0x30,0x32,0x30,0x36,
        0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        0x31,0x30,0x31,0x34,0x44,0x46,0x34,0x30,0x31,0x34,0x44,0x46,0x34};
    printf("\ncrc2(4AFF):%04X\n", Utils::Crc::Crc16_8005(buf2,sizeof(buf2)));
}

int main()
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    PrintVersion();
    try
    {
        srand(time(NULL));
#define LINKS_NTS 3 // from tcp-tsi-sp-003-nts
#define LINKS_WEB 2 // from web

        // 3(tmr) + 1+3*2(nts) + 1+2*2(web) + 7*2(com) + 1(led) = 30
        Epoll::Instance().Init(32);
        TimerEvent timerEvt10ms{10, "[tmrEvt10ms:10ms]"};
        TimerEvent timerEvt100ms{100, "[tmrEvt100ms:100ms]"};
        TimerEvent timerEvt1s{1000, "[tmrEvt1sec:1sec]"};
        timerEvt1s.Add(new TickTock{});
        StatusLed::Instance().Init(&timerEvt10ms);
        DbHelper::Instance().Init(&timerEvt1s);
        UciUser &user = DbHelper::Instance().uciUser;

        //AllGroupPowerOn();

        // init serial ports
        SerialPort *sp[COMPORT_SIZE];
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            sp[i] = nullptr;
        }

        // serial port init
        if (user.ComPort() == 0)
        {
            SerialPortConfig spCfg(SerialPortConfig::SpMode::RS232, user.Baudrate());
            sp[0] = new SerialPort(COM_DEV[0], spCfg);
        }
        else
        {
            SerialPortConfig spCfg(SerialPortConfig::SpMode::RS485_01, user.Baudrate());
            sp[user.ComPort()] = new SerialPort(COM_DEV[user.ComPort()], spCfg);
        }

        for (int i = 0; i < DbHelper::Instance().uciProd.NumberOfSigns(); i++)
        {
            struct StSignPort *cn = DbHelper::Instance().uciProd.SignPort(i);
            if (cn->com_ip < COMPORT_SIZE)
            { // com port
                if (sp[cn->com_ip] == nullptr)
                {
                    SerialPortConfig spCfg(SerialPortConfig::SpMode::RS485_01, cn->bps_port);
                    sp[cn->com_ip] = new SerialPort(COM_DEV[cn->com_ip], spCfg);
                }
                else
                {
                    if (sp[cn->com_ip]->Config().baudrate != cn->bps_port)
                    {
                        MyThrow("Sign%d baudrate error. Baudrate on %s should be same", i + 1, COM_NAME[cn->com_ip]);
                    }
                }
            }
            else
            { // ip
            }
        }

        Scheduler::Instance().Init(&timerEvt10ms);
        // TSI-SP-003 Web
        ObjectPool<OprTcp> webPool{LINKS_WEB};
        auto webpool = webPool.Pool();
        for (int i = 0; i < webPool.Size(); i++)
        {
            webpool[i].Init("Tcp" + std::to_string(i), "WEB", 300 * 1000);
        }
        TcpServer tcpServerWeb{user.WebPort(), webPool, &timerEvt1s};

        // TSI-SP-003 Tcp
        ObjectPool<OprTcp> ntsPool{LINKS_NTS};
        auto tcppool = ntsPool.Pool();
        for (int i = 0; i < ntsPool.Size(); i++)
        {
            tcppool[i].Init("Tcp" + std::to_string(i), "NTS", (user.SessionTimeout() + 60) * 1000);
        }
        TcpServer tcpServerPhcs{user.SvcPort(), ntsPool, &timerEvt1s};

        // TSI-SP-003 SerialPort
        OprSp  *oprSp[COMPORT_SIZE];
        
        oprSp[user.ComPort()] = new OprSp{*sp[user.ComPort()], COM_NAME[user.ComPort()], "NTS"};

        // slave com port
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            if(sp[i] != nullptr & i!=user.ComPort())
            {
                oprSp[i] = new OprSp{*sp[i], COM_NAME[i], "SLV"};
            }
        }
        
        /*************** Start ****************/
        while (1)
        {
            Epoll::Instance().EventsHandle();
        }
        /************* Never hit **************/
    }
    catch (const std::exception &e)
    {
        printf("main exception: %s\n", e.what());
        // clean
    }
    //muntrace();
}

// E-O-F
