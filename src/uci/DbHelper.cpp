#include <unistd.h>
#include <uci/DbHelper.h>

void DbHelper::Init()
{
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
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}

void DbHelper::RefreshSync()
{
    syncTmr.Setms(1000);
}

void DbHelper::Sync()
{
    if (syncTmr.IsExpired())
    {
        syncTmr.Clear();
        sync();
    }
}
