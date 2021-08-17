#include <uci/DbHelper.h>


DbHelper::~DbHelper()
{
    tmrEvt->Remove(this);
}

void DbHelper::Init(TimerEvent * tmr)
{
    uciFrm.LoadConfig();
    tmrEvt = tmr;
    tmrEvt->Add(this);
}

void DbHelper::PeriodicRun()
{
    if(sync)
    {
        sync=false;
    }
}

void DbHelper::Sync()
{
    sync=true;
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}

