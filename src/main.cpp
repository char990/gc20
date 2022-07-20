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
#include <layer/LayerNTS.h>
#include <layer/LayerWeb.h>
#include <layer/UI_LayerManager.h>
#include <layer/SLV_LayerManager.h>

#include <sign/Controller.h>
#include <module/Utils.h>
#include <module/DS3231.h>
#include <gpio/GpioIn.h>
#include <gpio/GpioOut.h>

#include <module/DebugConsole.h>

#include <websocket/WsServer.h>

#include <3rdparty/catch2/enable_test.h>

using namespace std;

const char *FirmwareVer = "0120";
const char *CONFIG_PATH = "config";

char *mainpath;


#ifdef DEBUG
const char *MAKE = "Debug";
#else
const char *MAKE = "Release";
#endif

#ifndef __BUILDTIME__
#define __BUILDTIME__ (__DATE__ " " __TIME__ " UTC")
#endif

int PrintfVersion_(bool start, char *buf)
{
    return snprintf(buf, 63, "%s* %s ver:%s @ %s *",
                    start ? ">>> START >>> " : "",
                    MAKE, FirmwareVer, __BUILDTIME__); // __BUILDTIME__ is defined in Makefile
}

void PrintVersion(bool start)
{
    char sbuf[64];
    int len = PrintfVersion_(start, sbuf);
    char buf[64];
    memset(buf, '*', len);
    buf[len] = '\0';
    Ldebug("%s", buf);
    Ldebug("%s", sbuf);
    Ldebug("%s", buf);
}

// a wrapper for time()
time_t GetTime(time_t *t)
{
    return time(t);
}

bool ticktock = true;
class TickTock : public IPeriodicRun
{
public:
#define RD_DS3231_SEC (3600 * 24)
    TickTock()
    {
        UpdateSysTime();
    }

    time_t UpdateSysTime()
    {
        time_t t1 = pDS3231->GetTimet();
        if (t1 > 0)
        {
            struct timeval t2;
            gettimeofday(&t2, nullptr);
            if (t2.tv_usec > 500000)
            {
                t2.tv_sec++;
            }
            if (t1 != t2.tv_sec)
            {
                struct tm stm;
                localtime_r(&t1, &stm);
                Ldebug("DS3231 updates system time->%2d/%02d/%d %2d:%02d:%02d",
                       stm.tm_mday, stm.tm_mon + 1, stm.tm_year + 1900, stm.tm_hour, stm.tm_min, stm.tm_sec);
                Utils::Time::SetLocalTime(stm);
            }
            else
            {
                Ldebug("DS3231 updates system time-> Timestamp matched, ignore this");
            }
        }
        return t1;
    }

    virtual void PeriodicRun() override
    {
        if (++cnt > RD_DS3231_SEC)
        {
            if (UpdateSysTime() > 0)
            {
                cnt = 0;
            }
        }
        pPinHeartbeatLed->Toggle();
        pPinWdt->Toggle();
        if ((cnt & 0x03) == 0)
        {
            pDS3231->WriteTimeAlarm(time(nullptr));
        }
        if (ticktock)
        {
            PrintDbg(DBG_HB, "%c   \x08\x08\x08", s[cnt & 0x03]);
            fflush(stdout);
        }
    };

private:
    int cnt{0};
    const char s[4]{'-', '\\', '|', '/'};
};

void LogResetTime()
{
    time_t t;
    pDS3231->ReadTimeAlarm(&t);
    auto &db = DbHelper::Instance();
    db.GetUciFault().Push(0, DEV::ERROR::ControllerResetViaWatchdog, 1, t);
    db.GetUciFault().Push(0, DEV::ERROR::ControllerResetViaWatchdog, 0);
    char buf[64];
    PrintfVersion_(true, buf);
    db.GetUciAlarm().Push(0, buf);
    db.GetUciEvent().Push(0, buf);
    return;
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
        Utils::Time::SleepMs(100);
    }
    Utils::Time::SleepMs(1000);
    pPinHeartbeatLed = new GpioOut(PIN_HB_LED, 1);  // heartbeat led, yellow
    pPinStatusLed = new GpioOut(PIN_ST_LED, 1);     // status led, green
    pPinWdt = new GpioOut(PIN_WDT, 1);              // watchdog
    pPinRelay = new GpioOut(PIN_RELAY_CTRL, 0);     // relay off
    pPinMosfet2 = new GpioOut(PIN_MOSFET2_CTRL, 0); // mosfet off
}

#ifdef CATCH2TEST
int test_mask_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    try
    {
        mainpath = get_current_dir_name();
        if (strlen(mainpath) > 64)
        {
            throw std::invalid_argument("path is longer than 64 bytes.\n");
        }
        PrintVersion(true);
        pDS3231 = new DS3231{1};
        TickTock *pTickTock = new TickTock{};

        srand(time(NULL));
        GpioInit();

        // 2(tmr) + 1+8*2(nts) + 1+4*2(web) + 7*2(com) + 1(led) + 1(debugconsole) = 44
        Epoll::Instance().Init(64);
        auto ctrllerTmrEvt = new TimerEvent{CTRLLER_TICK, "[ctrllerTmrEvt]"};
        auto timerEvt100ms = new TimerEvent{100, "[tmrEvt100ms:100ms]"};
        auto tmrEvt1Sec = new TimerEvent{1000, "[tmrEvt1Sec]"};
        tmrEvt1Sec->Add(pTickTock);
        auto console = new DebugConsole();

        DbHelper::Instance().Init(CONFIG_PATH);
        Controller::Instance().Init(ctrllerTmrEvt);

        LogResetTime();
        // AllGroupPowerOn();

        UciProd &prod = DbHelper::Instance().GetUciProd();
        UciUser &user = DbHelper::Instance().GetUciUser();
        // init serial ports
        OprSp *oprSp[COMPORT_SIZE];
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            oprSp[i] = nullptr;
        }
        // TSI-SP-003 RS232/485
        IUpperLayer *uiLayer = new UI_LayerManager(COM_NAME[user.ComPort()], "NTS");
        oprSp[user.ComPort()] = new OprSp{user.ComPort(), user.Baudrate(), uiLayer};
        if (prod.MonitoringPort() >= 0)
        {
            LayerWeb::monitor = LayerNTS::monitor = oprSp[prod.MonitoringPort()] =
                new OprSp{(uint8_t)prod.MonitoringPort(), prod.MonitoringBps(), nullptr, 1024 * 1024};
        }
        // Slaves
        if (SignCfg::bps_port > 0)
        { // Slaves of Groups on RS485
            for (int i = 1; i <= prod.NumberOfSigns(); i++)
            {
                auto &cn = prod.GetSignCfg(i);
                if (oprSp[cn.com_ip] == nullptr)
                {
                    auto g = Controller::Instance().GetGroup(cn.groupId);
                    IUpperLayer *upperLayer = new SLV_LayerManager(COM_NAME[cn.com_ip], g);
                    oprSp[cn.com_ip] = new OprSp{(uint8_t)cn.com_ip, SignCfg::bps_port, upperLayer};
                    g->SetOprSp(oprSp[cn.com_ip]);
                }
            }
        }
        else
        { // ip
            throw std::runtime_error("TODO: Slave on IP:Port");
        }

        // TSI-SP-003 Web
        // auto tcpServerWeb = new TcpServer {user.WebPort(), "WEB", prod.TcpServerWEB(), tmrEvt1Sec};
        auto wsServer = new WsServer{user.WebPort(), timerEvt100ms};
        // TSI-SP-003 Tcp
        auto tcpServerNts = new TcpServer{user.SvcPort(), "NTS", prod.TcpServerNTS(), tmrEvt1Sec};
        Controller::Instance().SetTcpServer(tcpServerNts);

        Ldebug(">>> DONE >>>");
        printf("\n=>Input '?<Enter>' to get console help.\n\n");
        /*************** Start ****************/
        while (1)
        {
            Epoll::Instance().EventsHandle();
        }
        /************* Never hit **************/
    }
    catch (const std::exception &e)
    {
        // muntrace();
        Ldebug("\n!!! main exception :%s", e.what());
        return 255;
        // clean
    }
    // muntrace();
}

// E-O-F
