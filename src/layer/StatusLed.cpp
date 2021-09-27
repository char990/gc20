#include <layer/StatusLed.h>
#include <uci/DbHelper.h>

StatusLed::StatusLed()
{
    sessionSt=0;
    dataSt=0;
    dataCnt=0;
    sessionTmr.Clear();
}

StatusLed::~StatusLed()
{
    if(tmrEvt)
    {
        tmrEvt->Remove(this);
    }
}

void StatusLed::Init(TimerEvent *tmrEvt1)
{
    tmrEvt = tmrEvt1;
    tmrEvt->Add(this);
}

void StatusLed::PeriodicRun()
{
    if(sessionTmr.IsExpired())
    {
        // clr led
    }
    if(dataSt)
    {
        dataCnt++;
        if(dataCnt==1)
        {
// set led
        }
        else if(dataCnt==11)
        {
// clr led
        }
        else if(dataCnt==21)
        {
            dataCnt=0;
            dataSt=0;
        }
    }
}

void StatusLed::ReloadSessionSt()
{
    sessionTmr.Setms(DbHelper::Instance().GetUciUser().SessionTimeout()*1000);
    // set led
}

void StatusLed::ReloadDataSt()
{
    dataSt=1;
}
