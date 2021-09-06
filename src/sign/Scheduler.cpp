#include <sign/Scheduler.h>
#include <sign/Vms.h>
#include <sign/Islus.h>
#include <module/ptcpp.h>

Scheduler::Scheduler()
:signs(nullptr),groups(nullptr),tmrEvt(nullptr)
{
}

Scheduler::~Scheduler()
{
    if(groups != nullptr)
    {
        for(int i=0;i<groupCnt;i++)
        {
            delete groups[i];
        }
        delete [] groups;
    }
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

void Scheduler::Init(TimerEvent *tmrEvt1)
{
    if(tmrEvt1!=nullptr)
    {
        MyThrow ( "Re-invoke Scheduler::Init()" );
    }
    tmrEvt=tmrEvt1;
    tmrEvt->Add(this);
    displayTimeout.Clear();

    // sign init
    int signCnt=DbHelper::Instance().uciProd.NumberOfSigns();
    signs = new Sign * [signCnt];
    switch(DbHelper::Instance().uciProd.SignType())
    {
        case 0:
            for(int i=0;i<signCnt;i++)
            {
                signs[i] = new Vms(i+1);
            }
        break;
        case 1:
            for(int i=0;i<signCnt;i++)
            {
                signs[i] = new Islus(i+1);
            }
        break;
    }

    // group init
    uint8_t *groupcfg = DbHelper::Instance().uciUser.GroupCfg();
    groupCnt=0;
    for(int i=0;i<signCnt;i++)
    {
        if(groupcfg[i]>groupCnt)
        {
            groupCnt=groupcfg[i];
        }
    }
    groups = new Group * [groupCnt];
    for(int i=0;i<groupCnt;i++)
    {
        groups[i] = new Group(i+1);
    }
    for(int i=0;i<signCnt;i++)
    {
        groups[groupcfg[i]]->Add(signs[i]);
    }
    for(int i=0;i<groupCnt;i++)
    {
        if(groups[i].GrpSigns().size==0)
        {
            MyThrow("Error:There is no sign in Group[%d]", i+1);
        }
    }
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

