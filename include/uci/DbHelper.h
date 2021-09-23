#ifndef __DBHELPER_H__
#define __DBHELPER_H__

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

    UciProd uciProd;
    UciUser uciUser;

    UciFrm uciFrm;
    UciMsg uciMsg;
    UciPln uciPln;

    UciFault uciFlt;
    UciAlarm uciAlm;
    UciEvent uciEvt;

    UciProcess uciProcess;

    void RefreshSync();

protected:
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


#endif
