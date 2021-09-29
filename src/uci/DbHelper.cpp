#include <unistd.h>
#include <cstdio>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

void DbHelper::Init(TimerEvent *tmrEvt1, const char * dbPath1)
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
    syncTmr.Clear();
    PrintDash();
    tmrEvt=tmrEvt1;
    tmrEvt->Add(this);
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}

void DbHelper::RefreshSync()
{
    syncTmr.Setms(3000);
}

void DbHelper::PeriodicRun()
{
    if (syncTmr.IsExpired())
    {
        syncTmr.Clear();
        sync();
    }
}
