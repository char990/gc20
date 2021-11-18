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
#include <layer/UI_LayerManager.h>
#include <layer/SLV_LayerManager.h>

#include <sign/Controller.h>
#include <module/Utils.h>
#include <module/DS3231.h>
#include <gpio/GpioIn.h>
#include <gpio/GpioOut.h>

#include <module/DebugConsole.h>

const char *FirmwareMajorVer = "01";
const char *FirmwareMinorVer = "50";
const char *CONFIG_PATH = "config";

char *mainpath;

using namespace std;

void PrintVersion()
{
    char sbuf[256];
    int len = snprintf(sbuf, 255, "* Version %s.%s, Build at UTC: %s %s *",
                       FirmwareMajorVer, FirmwareMinorVer, __DATE__, __TIME__);
    char buf[256];
    memset(buf, '*', len);
    buf[len] = '\0';
    printf("%s\n", buf);
    printf("%s\n", sbuf);
    printf("%s\n", buf);
}

bool ticktock = true;
class TickTock : public IPeriodicRun
{
public:
    virtual void PeriodicRun() override
    {
        if (ticktock)
        {
            putchar('\r');
            _r_need_n = 0;
            PrintDbg(DBG_0, "%c   \x08\x08\x08", s[cnt & 0x03]);
            _r_need_n = 1;
            cnt++;
            fflush(stdout);
        }
        time_t alarm_t = time(NULL);
        pDS3231->WriteTimeAlarm(alarm_t);
        pPinHeartbeatLed->Toggle();
    };

private:
    uint8_t cnt{0};
    char s[4]{'-', '\\', '|', '/'};
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

void TestDebounce()
{
    Debounce test(5, 4);
    char c;
    while (1)
    {
        test.State();
        scanf(" %c", &c);
        switch (c)
        {
        case '0':
        case '1':
            test.Check(c != '0');
            break;
        case 'r':
            test.ClearRising();
            break;
        case 'f':
            test.ClearFalling();
            break;
        default:
            test.ClearEdge();
            break;
        }
    }
}

void LogResetTime()
{
    if (access(".lrt", F_OK) != 0)
    { // there is no ".lrt"
        time_t t;
        t = time(nullptr);
        pDS3231->ReadTimeAlarm(&t);
        auto &db = DbHelper::Instance();
        db.GetUciFault().Push(0, DEV::ERROR::ControllerResetViaWatchdog, 1, t);
        db.GetUciFault().Push(0, DEV::ERROR::ControllerResetViaWatchdog, 0);
        db.GetUciAlarm().Push(0, "<--- Reset --->");
        db.GetUciEvent().Push(0, "<--- Reset --->");
    }
    else
    { // ".lrt" exists
        remove(".lrt");
    }
}

void GpioInit()
{
    unsigned int pins[] = {
        PIN_CN9_7,
        PIN_CN9_8,
        PIN_CN9_9,
        PIN_CN9_10,
        PIN_CN7_1,
        PIN_CN7_2,
        PIN_CN7_3,
        PIN_CN7_4,
        PIN_CN7_7,
        PIN_CN7_8,
        PIN_CN7_9,
        PIN_CN7_10,
        PIN_IN1,
        PIN_IN2,
        PIN_IN3,
        PIN_IN4,
        PIN_IN5,
        PIN_IN6,
        PIN_IN7,
        PIN_IN8,
        PIN_CN9_2,
        PIN_CN9_4,
        PIN_HB_LED,
        PIN_ST_LED,
        PIN_RELAY_CTRL,
        PIN_WDT};

    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        GpioEx::Export(pins[i]);
    }
    Utils::Time::SleepMs(1000);
    pPinHeartbeatLed = new GpioOut(PIN_HB_LED, 1);  // heartbeat led, yellow
    pPinStatusLed = new GpioOut(PIN_ST_LED, 1);     // status led, green
    pPinWdt = new GpioOut(PIN_WDT, 1);              // watchdog
    pPinRelay = new GpioOut(PIN_RELAY_CTRL, 0);     // relay off
    pPinMosfet2 = new GpioOut(PIN_MOSFET2_CTRL, 0); // mosfet off
}

int main(int argc, char *argv[])
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    try
    {
        mainpath = get_current_dir_name();
        if (strlen(mainpath) > 64)
        {
            MyThrow("path is longer than 64 bytes.\n");
        }
        PrintVersion();

        PrintDbg(DBG_LOG, "\n>>> %s START... >>>\n", argv[0]);
        srand(time(NULL));
        GpioInit();

        pDS3231 = new DS3231{1};

#define LINKS_NTS 3 // from tcp-tsi-sp-003-nts
#define LINKS_WEB 2 // from web
        // 2(tmr) + 1+3*2(nts) + 1+2*2(web) + 7*2(com) + 1(led) + 1(debugconsole) = 30
        Epoll::Instance().Init(32);
        TimerEvent ctrllerTmrEvt{CTRLLER_TICK, "[ctrllerTmrEvt]"};
        //TimerEvent timerEvt100ms{100, "[tmrEvt100ms:100ms]"};
        TimerEvent tmrEvt1Sec{1000, "[tmrEvt1Sec]"};
        tmrEvt1Sec.Add(new TickTock{});
        auto console = new DebugConsole();

        DbHelper::Instance().Init(CONFIG_PATH);
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
                            Controller::Instance().GetGroup(g)->SetOprSp(oprSp[cn->com_ip]);
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
        TcpServer tcpServerWeb{user.WebPort(), "WEB", LINKS_WEB, &tmrEvt1Sec};

        // TSI-SP-003 Tcp
        TcpServer tcpServerNts{user.SvcPort(), "NTS", LINKS_NTS, &tmrEvt1Sec};
        Controller::Instance().SetTcpServer(&tcpServerNts);

        PrintDbg(DBG_LOG, ">>> DONE >>>\n");

        /*************** Start ****************/
        while (1)
        {
            Epoll::Instance().EventsHandle();
        }
        /************* Never hit **************/
    }
    catch (const std::exception &e)
    {
        //muntrace();
        PrintDbg(DBG_LOG, "\n!!! main exception :%s\n", e.what());
        exit(1);
        // clean
    }
    //muntrace();
}

// E-O-F
