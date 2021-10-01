/*
 *
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <mcheck.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <module/Epoll.h>
#include <module/SerialPort.h>
#include <module/TimerEvent.h>
#include <uci/DbHelper.h>
#include <module/TcpServer.h>
#include <module/OprTcp.h>
#include <module/OprSp.h>
#include <module/ObjectPool.h>
#include <layer/UI_LayerManager.h>
#include <layer/SLV_LayerManager.h>

#include <layer/StatusLed.h>
#include <sign/Scheduler.h>
#include <module/Utils.h>

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
    printf("* Version %s.%s, Build at UTC %s %s *\n",
           FirmwareMajorVer, FirmwareMinorVer, __DATE__, __TIME__);
    PrintBorder();
}

class TickTock : public IPeriodicRun
{
public:
    virtual void PeriodicRun() override
    {
        char buf[20];
        Utils::Cnvt::ParseTmToLocalStr(time(nullptr), buf);
        printf("\r[%s]", buf);
        fflush(stdout);
    };
};

void TestCrc8005()
{
    uint8_t buf1[] = {0x02, 0x30, 0x31, 0x30, 0x35};
    printf("\ncrc1(18F3):%04X\n", Utils::Crc::Crc16_8005(buf1, sizeof(buf1)));
    uint8_t buf2[] = {
        0x02, 0x30, 0x32, 0x30, 0x36,
        0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
        0x31, 0x30, 0x31, 0x34, 0x44, 0x46, 0x34, 0x30, 0x31, 0x34, 0x44, 0x46, 0x34};
    printf("\ncrc2(4AFF):%04X\n", Utils::Crc::Crc16_8005(buf2, sizeof(buf2)));
}

int main(int argc, char *argv[])
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    PrintVersion();
    if(argc !=2)
    {
        printf("Usage: %s DIRECTORY\n", argv[0]);
        return 1;
    }
    {
        struct stat st;
        if(stat(argv[1], &st) == 0)
        {
            if(!S_ISDIR(st.st_mode))
            {
                printf("'%s' is NOT a directory\n", argv[1]);
                exit(3);
            }
        }
        else
        {
            printf("'%s' does NOT exist\n", argv[1]);
            exit(2);
        }
    }

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
        DbHelper::Instance().Init(&timerEvt1s, argv[1]);
        UciProd & prod = DbHelper::Instance().GetUciProd();
        UciUser & user = DbHelper::Instance().GetUciUser();
        Scheduler::Instance().Init(&timerEvt10ms);

        //AllGroupPowerOn();

        // init serial ports
        OprSp* oprSp[COMPORT_SIZE];
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            oprSp[i] = nullptr;
        }
        // TSI-SP-003 RS232/485
        IUpperLayer *uiLayer = new UI_LayerManager(gSpConfig[user.ComPort()].name, "NTS");
        oprSp[user.ComPort()] = new OprSp{user.ComPort(), user.Baudrate(), uiLayer};
        // Slaves of Groups on RS485
        for (int i = 1; i <= prod.NumberOfSigns(); i++)
        {
            struct StSignPort *cn = prod.SignPort(i);
            if (cn->com_ip < COMPORT_SIZE)
            { // com port
                if (oprSp[cn->com_ip] == nullptr)
                {
                    for (uint8_t g = 1; g <= Scheduler::Instance().GroupCnt(); g++)
                    {
                        if (Scheduler::Instance().GetGroup(g)->IsSignInGroup(i))
                        {
                            IUpperLayer *upperLayer = new SLV_LayerManager(gSpConfig[cn->com_ip].name, Scheduler::Instance().GetGroup(g));
                            oprSp[cn->com_ip] = new OprSp{(uint8_t)cn->com_ip, cn->bps_port, upperLayer};
                        }
                    }
                }
                else
                {
                    if (oprSp[cn->com_ip]->Bps() != cn->bps_port)
                    {
                        MyThrow("Sign%d baudrate error. Baudrate on %s should be same", i + 1, oprSp[cn->com_ip]->Name());
                    }
                }
            }
            else
            { // ip
            }
        }

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
