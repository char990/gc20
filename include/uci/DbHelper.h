#pragma once


#include <uci/UciProd.h>
#include <uci/UciUserCfg.h>
#include <uci/UciFrm.h>
#include <uci/UciMsg.h>
#include <uci/UciPln.h>
#include <uci/UciFault.h>
#include <uci/UciAlarm.h>
#include <uci/UciEvent.h>
#include <uci/UciProcess.h>
#include <uci/UciNetwork.h>
#include <uci/UciPasswd.h>
#include <module/BootTimer.h>


class DbHelper
{
public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance;
        return instance;
    }

    void Init(const char * dbPath);
    
    uint16_t HdrChksum();

    UciProd & GetUciProd() { return uciProd; };
    UciUserCfg & GetUciUser() { return uciUserCfg; };
    UciNetwork & GetUciNetwork() {return uciNetwork;};

    UciFrm & GetUciFrm() { return  uciFrm; };
    UciMsg & GetUciMsg() { return uciMsg; };
    UciPln & GetUciPln() { return uciPln; };
    
    UciFault & GetUciFault() { return uciFlt; };
    UciAlarm & GetUciAlarm() { return uciAlm; };
    UciEvent & GetUciEvent() { return uciEvt; };
    
    UciProcess & GetUciProcess() { return uciProcess; };

    UciPasswd & GetUciPasswd() { return uciPasswd; };

    const char * Path(){ return dbPath; };

protected:
    UciProd uciProd;
    UciUserCfg uciUserCfg;
    UciNetwork uciNetwork;

    UciFrm uciFrm;
    UciMsg uciMsg;
    UciPln uciPln;

    UciFault uciFlt;
    UciAlarm uciAlm;
    UciEvent uciEvt;

    UciProcess uciProcess;

    UciPasswd uciPasswd;

private:
    DbHelper(){}
    ~DbHelper(){}

    const char * dbPath;
};

