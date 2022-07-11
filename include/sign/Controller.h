#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <module/BootTimer.h>
#include <module/IPeriodicRun.h>
#include <module/TimerEvent.h>
#include <sign/Sign.h>
#include <sign/Group.h>
#include <tsisp003/TsiSp003Const.h>
#include <gpio/GpioIn.h>
#include <module/TcpServer.h>
#include <module/Debounce.h>

#define RQST_NETWORK (1 << 0)
#define RQST_RESTART (1 << 1)
#define RQST_REBOOT (1 << 2)

class Controller : public IPeriodicRun
{
public:
    Controller(Controller const &) = delete;
    void operator=(Controller const &) = delete;
    static Controller &Instance()
    {
        static Controller instance;
        return instance;
    }

    /*< IPeriodicRun -------------------------------------------*/
    /// \brief  Called by TimerEvt
    virtual void PeriodicRun() override;
    /*--------------------------------------------------------->*/

    void Init(TimerEvent *tmrEvt);

    void SetTcpServer(TcpServer *tcpServer);

    void RefreshDispTime();

    void RefreshSessionTime();

    uint8_t CtrllerErr();

    uint8_t GroupCnt() { return groups.size(); };
    Group *GetGroup(uint8_t id) { return (id == 0 || id > groups.size()) ? nullptr : groups.at(id - 1); };
    std::vector<Group *> &GetGroups() { return groups; };

    bool IsFrmActive(uint8_t i);
    bool IsMsgActive(uint8_t i);
    bool IsPlnActive(uint8_t i);
    bool IsPlnEnabled(uint8_t id);
    void SetOnline(bool v){isOnline=v;};
    bool IsOnline(){return isOnline;};

    // command from TSI-SP-003/WebSocket
    APP::ERROR CheckAdjacentLane(std::vector<uint8_t> & frms);
    APP::ERROR CmdDispFrm(uint8_t *cmd);
    APP::ERROR CmdDispMsg(uint8_t *cmd);
    APP::ERROR CmdDispAtomicFrm(uint8_t *cmd, int len);
    APP::ERROR CmdSignTest(uint8_t *cmd);

    APP::ERROR SignSetFrame(uint8_t *data, int len, char *rejectStr);

    int CmdRequestEnabledPlans(uint8_t *buf);
    APP::ERROR CmdEnDisPlan(uint8_t *cmd);

    APP::ERROR CmdSetDimmingLevel(uint8_t *cmd, char *rejectStr);
    APP::ERROR CmdPowerOnOff(uint8_t *cmd, char *rejectStr);
    APP::ERROR CmdDisableEnableDevice(uint8_t *cmd, char * rejectStr);

    APP::ERROR CmdSystemReset(uint8_t *cmd, char *rejectStr);

    APP::ERROR CmdUpdateTime(struct tm & stm);

    CtrllerError ctrllerError;
    BootTimer sessionTimeout;

    int8_t CurTemp() { return curTemp; };
    int8_t MaxTemp() { return maxTemp; };

    void RR_flag(uint8_t rr)
    {
        rr_flag |= rr;
        if (rr_flag != 0)
        {
            rrTmr.Setms(5000);
        }
    };

    std::vector<Group *> groups;
    std::vector<Sign *> signs;



private:
    Controller();
    ~Controller();
    DbHelper &db;

    bool isOnline{false};

    TimerEvent *tmrEvt{nullptr};

    /// \brief Display timeout timer
    BootTimer displayTimeout;

    void PowerMonitor();
    GpioIn *pMainPwr;
    GpioIn *pBatLow;
    GpioIn *pBatOpen;
    BootTimer ms100Tmr;
    const unsigned int EXTIN_PINS[4]{PIN_MSG3, PIN_MSG4, PIN_MSG5, PIN_MSG6};
    std::vector<GpioIn *> extInput{4};
    void ExtInputFunc();

    BootTimer tempTmr;
    int8_t curTemp{0};
    int8_t maxTemp{0};
    Debounce overtempFault;

    // restart/reboot/network flag
    uint8_t rr_flag{0};
    BootTimer rrTmr;

    TcpServer *tcpServer;
//    void WriteFifo(char c);
};
