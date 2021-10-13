#include <unistd.h>
#include <cstdio>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

void DbHelper::Init(const char * dbPath1)
{
    dbPath = dbPath1;
    uciProd.LoadConfig();
    uciUser.LoadConfig();
    uciFrm.LoadConfig();
    uciMsg.LoadConfig();
    uciPln.LoadConfig();
    uciProcess.LoadConfig();
    uciFlt.LoadConfig();
    uciAlm.LoadConfig();
    uciEvt.LoadConfig();
    PrintDash();
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}
