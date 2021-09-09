#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <cstdint>
#include <string>
#include <module/BootTimer.h>
#include <module/IPeriodicRun.h>
#include <module/TimerEvent.h>
#include <sign/IUnitedSign.h>
#include <sign/Group.h>

class Scheduler : public IPeriodicRun
{
public:
    Scheduler(Scheduler const &) = delete;
    void operator=(Scheduler const &) = delete;
    static Scheduler &Instance()
    {
        static Scheduler instance;
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
    
    IUnitedSign ** unitedSigns;
    Group ** groups;


private:
    Scheduler();
    ~Scheduler();

    TimerEvent *tmrEvt;

    /// \brief Display timeout timer
    BootTimer displayTimeout;

    uint8_t
        sessionLed,
        ctrllerErr,
        groupCnt,
        signCnt;

};

#endif
