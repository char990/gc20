#ifndef __DBHELPER_H__
#define __DBHELPER_H__

#include <module/SignCfg.h>
#include <uci/UciProd.h>
#include <uci/UciUser.h>
#include <uci/UciFrm.h>
#include <uci/UciMsg.h>
#include <uci/UciPln.h>


class DbHelper
{
private:
    DbHelper() {};

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

    UciProd prodCfg;
    UciUser userCfg;
    SignCfg signCfg;
    UciFrm uciFrm;
    UciMsg uciMsg;
    UciPln uciPln;
};

#endif
