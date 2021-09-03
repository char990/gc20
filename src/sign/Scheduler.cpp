#include <sign/Scheduler.h>
#include <sign/SignHrg.h>
#include <sign/SignGfx.h>
#include <sign/SignTxt.h>
#include <module/ptcpp.h>



Scheduler::Scheduler()
:signs(nullptr),tmrEvt(nullptr)
{
}

Scheduler::~Scheduler()
{
    if(signs != nullptr)
    {
        for(int i=0;i<signCnt;i++)
        {
            delete signs[i];
        }
        delete [] signs;
    }
    if(tmrEvt != nullptr)
    {
        tmrEvt->Remove(this);
    }
}

void Scheduler::Init(TimerEvent *tmrEvt)
{
    if(tmrEvt!=nullptr)
    {
        MyThrow ( "Re-invoke Scheduler::Init()" );
    }
    this->tmrEvt=tmrEvt;
    tmrEvt->Add(this);
    displayTimeout.Clear();
    UciProd &prod = DbHelper::Instance().uciProd;
    int signCnt=prod.NumberOfSigns();
    Sign::signCfg.MakeSignCfg(prod.SignType());

    signs = new Sign * [signCnt];
    for(int i=0;i<signCnt;i++)
    {
        if()
        //auto s = ;
        signs[i] = new SignTxt();
    }
    signs[0]->SetId(1, 1);
    signs[1]->SetId(2, 1);
    signs[2]->SetId(3, 2);
    signs[3]->SetId(4, 2);
}

void Scheduler::PeriodicRun()
{// run every 10ms




    if(displayTimeout.IsExpired())
    {
        displayTimeout.Clear();





    }
}

void Scheduler::RefreshDispTime()
{
    displayTimeout.Setms(100000);
}

void Scheduler::SessionLed(uint8_t v)
{
    sessionLed = v;
}

uint8_t Scheduler::ErrorCode()
{
    return errorCode;
}

