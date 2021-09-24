#pragma once

#include <module/IPeriodicRun.h>
#include <module/TimerEvent.h>
#include <module/BootTimer.h>

class StatusLed : public IPeriodicRun
{
public:
    StatusLed(StatusLed const &) = delete;
    void operator=(StatusLed const &) = delete;
    static StatusLed &Instance()
    {
        static StatusLed instance;
        return instance;
    }

    void Init(TimerEvent *tmrEvt);

    void PeriodicRun() override;

    void ReloadSessionSt();
    void ReloadDataSt();

    // note : current sessionSt is not good

private:
    StatusLed();
    ~StatusLed();
    TimerEvent *tmrEvt;
    uint8_t sessionSt;
    uint8_t dataSt, dataCnt;
    BootTimer sessionTmr;
};

