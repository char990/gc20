#ifndef __DBHELPER_H__
#define __DBHELPER_H__

#include <module/IPeriodicRun.h>
#include <module/SignCfg.h>
#include <uci/UciProd.h>
#include <uci/UciUser.h>
#include <uci/UciFrm.h>
#include <uci/UciMsg.h>
#include <uci/UciPln.h>
#include <module/TimerEvent.h>

class DbHelper: public IPeriodicRun
{
public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance;
        return instance;
    }

    /*< IPeriodicRun --------------------------------------------------*/
    /// \brief  Called by TimerEvt
    virtual void PeriodicRun() override;
    /*--------------------------------------------------------->*/

    void Init(TimerEvent * tmr);

    uint16_t HdrChksum();

    void Sync();

    UciProd prodCfg;
    UciUser userCfg{prodCfg};
    SignCfg signCfg;

    UciFrm uciFrm;
    UciMsg uciMsg{uciFrm};
    UciPln uciPln{uciFrm,uciMsg};

protected:


private:
    DbHelper():sync(false){};
    ~DbHelper();    
    bool sync;
    TimerEvent * tmrEvt;
};

#endif
