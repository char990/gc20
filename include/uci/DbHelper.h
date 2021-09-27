#pragma once


#include <uci/UciProd.h>
#include <uci/UciUser.h>
#include <uci/UciFrm.h>
#include <uci/UciMsg.h>
#include <uci/UciPln.h>
#include <uci/UciFault.h>
#include <uci/UciAlarm.h>
#include <uci/UciEvent.h>
#include <uci/UciProcess.h>
#include <module/BootTimer.h>
#include <module/TimerEvent.h>

class DbHelper : public IPeriodicRun
{
public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance;
        return instance;
    }

    void Init(TimerEvent *tmrEvt);

    virtual void PeriodicRun() override;
    
    uint16_t HdrChksum();

    void RefreshSync();

    UciProd & GetUciProd() { return uciProd; };
    UciUser & GetUciUser() { return uciUser; };

    UciFrm & GetUciFrm() { return  uciFrm; };
    UciMsg & GetUciMsg() { return uciMsg; };
    UciPln & GetUciPln() { return uciPln; };
    
    UciFault & GetUciFault() { return uciFlt; };
    UciAlarm & GetUciAlarm() { return uciAlm; };
    UciEvent & GetUciEvent() { return uciEvt; };
    
    UciProcess & GetUciProcess() { return uciProcess; };

protected:
    UciProd uciProd;
    UciUser uciUser;

    UciFrm uciFrm;
    UciMsg uciMsg;
    UciPln uciPln;

    UciFault uciFlt;
    UciAlarm uciAlm;
    UciEvent uciEvt;

    UciProcess uciProcess;

private:
    DbHelper():tmrEvt(nullptr){};
    ~DbHelper()
    {
        if(tmrEvt!=nullptr)
        {
            tmrEvt->Remove(this);
        }
    };
    BootTimer syncTmr;
    TimerEvent *tmrEvt;
};

