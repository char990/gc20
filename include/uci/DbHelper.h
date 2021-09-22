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

    void Init();
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

protected:
private:
    DbHelper(){};
    ~DbHelper();
};

#endif
