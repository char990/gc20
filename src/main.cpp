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
#include <sys/time.h>

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

#include <sign/Controller.h>
#include <module/Utils.h>
#include <module/DS3231.h>
#include <gpio/GpioIn.h>
#include <gpio/GpioOut.h>

const char *FirmwareMajorVer = "01";
const char *FirmwareMinorVer = "50";

using namespace std;

void PrintVersion()
{
    PrintDbg("\n");
    char sbuf[256];
    int len = sprintf(sbuf, "* Version %s.%s, Build at UTC: %s %s *",
                      FirmwareMajorVer, FirmwareMinorVer, __DATE__, __TIME__);
    char buf[256];
    memset(buf, '*', len);
    buf[len] = '\0';
    printf("%s\n", buf);
    printf("%s\n", sbuf);
    printf("%s\n", buf);
}

class TickTock : public IPeriodicRun
{
public:
    virtual void PeriodicRun() override
    {
        putchar('\r');
        _r_need_n = 0;
        PrintDbg("");
        _r_need_n = 1;
        cnt++;
        putchar(s[cnt&0x03]);
        fflush(stdout);
        time_t alarm_t = time(NULL);
        pDS3231->WriteTimeAlarm(alarm_t);
        pPinHeartbeatLed->Toggle();
    };
private:
    uint8_t cnt{0};
    char s[4]{'-','\\','|','/'};
};

/*
void TestCrc8005()
{
    uint8_t buf1[] = {0x02, 0x30, 0x31, 0x30, 0x35};
    PrintDbg("\ncrc1(18F3):%04X\n", Utils::Crc::Crc16_8005(buf1, sizeof(buf1)));
    uint8_t buf2[] = {
        0x02, 0x30, 0x32, 0x30, 0x36,
        0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
        0x31, 0x30, 0x31, 0x34, 0x44, 0x46, 0x34, 0x30, 0x31, 0x34, 0x44, 0x46, 0x34};
    PrintDbg("\ncrc2(4AFF):%04X\n", Utils::Crc::Crc16_8005(buf2, sizeof(buf2)));
}
*/

void LogResetTime()
{
    time_t t;
    pDS3231->ReadTimeAlarm(&t);
    DbHelper::Instance().GetUciFault().Push(0, DEV::ERROR::ControllerResetViaWatchdog, 1, t);
    DbHelper::Instance().GetUciFault().Push(0, DEV::ERROR::ControllerResetViaWatchdog, 0);
}

void GpioInit()
{
    pPinHeartbeatLed = new GpioOut(PIN_HB_LED, 1); // heartbeat led, yellow

    pPinStatusLed = new GpioOut(PIN_ST_LED, 1); // status led, green

    pPinWdt = new GpioOut(PIN_WDT, 1); // watchdog

    pPinRelay = new GpioOut(PIN_RELAY_CTRL, 0); // relay off

    pPinMosfet2 = new GpioOut(PIN_MOSFET2_CTRL, 0); // mosfet off
}

int main(int argc, char *argv[])
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    PrintVersion();
    if (argc != 2)
    {
        printf("Usage: %s DIRECTORY\n", argv[0]);
        return 1;
    }
    else
    {
        int x = strlen(argv[1]);
        if (argv[1][x - 1] == '/')
        {
            argv[1][x - 1] = '\0'; // remove last'/'
        }
        struct stat st;
        if (stat(argv[1], &st) != 0)
        {
            printf("'%s' does NOT exist\n", argv[1]);
            return 2;
        }
        else
        {
            if (!S_ISDIR(st.st_mode))
            {
                printf("'%s' is NOT a directory\n", argv[1]);
                return 3;
            }
        }
    }

    try
    {
        srand(time(NULL));

        pDS3231 = new DS3231{1};

#define LINKS_NTS 3 // from tcp-tsi-sp-003-nts
#define LINKS_WEB 2 // from web
        // 3(tmr) + 1+3*2(nts) + 1+2*2(web) + 7*2(com) + 1(led) = 30
        Epoll::Instance().Init(32);
        TimerEvent ctrllerTmrEvt{CTRLLER_TICK, "[ctrllerTmrEvt]"};
        //TimerEvent timerEvt100ms{100, "[tmrEvt100ms:100ms]"};
        TimerEvent tmrEvt1Sec{1000, "[tmrEvt1Sec]"};
        tmrEvt1Sec.Add(new TickTock{});

        DbHelper::Instance().Init(argv[1]);
        GpioInit();
        Controller::Instance().Init(&ctrllerTmrEvt);

        LogResetTime();
        //AllGroupPowerOn();

        UciProd &prod = DbHelper::Instance().GetUciProd();
        UciUser &user = DbHelper::Instance().GetUciUser();
        // init serial ports
        OprSp *oprSp[COMPORT_SIZE];
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
                    for (uint8_t g = 1; g <= Controller::Instance().GroupCnt(); g++)
                    {
                        if (Controller::Instance().GetGroup(g)->IsSignInGroup(i))
                        {
                            IUpperLayer *upperLayer = new SLV_LayerManager(gSpConfig[cn->com_ip].name, Controller::Instance().GetGroup(g));
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
        TcpServer tcpServerWeb{user.WebPort(), webPool, &tmrEvt1Sec};

        // TSI-SP-003 Tcp
        ObjectPool<OprTcp> ntsPool{LINKS_NTS};
        auto tcppool = ntsPool.Pool();
        for (int i = 0; i < ntsPool.Size(); i++)
        {
            tcppool[i].Init("Tcp" + std::to_string(i), "NTS", (user.SessionTimeout() + 60) * 1000);
        }
        TcpServer tcpServerPhcs{user.SvcPort(), ntsPool, &tmrEvt1Sec};

        PrintDbg("\n>>> START >>>\n");

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
