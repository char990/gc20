#ifndef __DBHELPER_H__
#define __DBHELPER_H__

#include <uci/UciProd.h>
#include <uci/UciUser.h>
#include <uci/UciFrm.h>
#include <uci/UciMsg.h>
#include <uci/UciPln.h>

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

    void Init();
    uint16_t HdrChksum();

    UciProd uciProd;
    UciUser uciUser{uciProd};

    UciFrm uciFrm;
    UciMsg uciMsg{uciFrm};
    UciPln uciPln{uciFrm,uciMsg};
protected:


private:
    DbHelper(){};
    ~DbHelper();    
};

#endif
