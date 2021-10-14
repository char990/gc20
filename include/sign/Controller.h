#pragma once


#include <cstdint>
#include <string>
#include <module/BootTimer.h>
#include <module/IPeriodicRun.h>
#include <module/TimerEvent.h>
#include <sign/Sign.h>
#include <sign/Group.h>
#include <tsisp003/TsiSp003Const.h>

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

    void RefreshDispTime();

    void SessionLed(uint8_t v);

    uint8_t CtrllerErr();

    uint8_t GroupCnt() { return groupCnt; };
    Group *GetGroup(uint8_t id) { return (id==0 || id>groupCnt) ? nullptr : groups[id - 1]; };

    bool IsFrmActive(uint8_t i);
    bool IsMsgActive(uint8_t i);
    bool IsPlnActive(uint8_t i);

    // cmaand from TSI-SP-003
    APP::ERROR CmdDispFrm(uint8_t *cmd);
    APP::ERROR CmdDispMsg(uint8_t *cmd);
    APP::ERROR CmdDispAtomicFrm(uint8_t *cmd, int len);

    int CmdRequestEnabledPlans(uint8_t * buf);
    APP::ERROR CmdEnDisPlan(uint8_t *cmd);

    APP::ERROR CmdSetDimmingLevel(uint8_t *cmd);
    APP::ERROR CmdPowerOnOff(uint8_t *cmd, int len);
    APP::ERROR CmdDisableEnableDevice(uint8_t *cmd, int len);

    APP::ERROR CmdSystemReset(uint8_t *cmd);

    CtrllerError ctrllerError;

private:
    Controller();
    ~Controller();

    Group **groups{nullptr};

    TimerEvent *tmrEvt{nullptr};

    /// \brief Display timeout timer
    BootTimer displayTimeout;

    uint8_t
        sessionLed{0},
        groupCnt{0};

};
