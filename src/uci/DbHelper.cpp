#include <unistd.h>
#include <cstdio>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

void DbHelper::Init(const char * dbPath1)
{
    dbPath = dbPath1;
    uciProd.LoadConfig();
    uciUser.LoadConfig();
    uciNetwork.LoadConfig();
    uciFrm.LoadConfig();
    uciMsg.LoadConfig();
    uciPln.LoadConfig();
    uciProcess.LoadConfig();
    uciFlt.LoadConfig();
    uciAlm.LoadConfig();
    uciEvt.LoadConfig();
    uciPasswd.LoadConfig();
    PrintDash('-');
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}
