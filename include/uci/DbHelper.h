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
    UciUser uciUser{uciProd};

    UciFrm uciFrm{uciProd};
    UciMsg uciMsg;
    UciPln uciPln;

    UciFault uciFlt;
    UciAlarm uciAlm;
    UciEvent uciEvt;

protected:
private:
    DbHelper(){};
    ~DbHelper();
};

#endif
