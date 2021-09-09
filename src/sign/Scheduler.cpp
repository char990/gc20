#include <sign/Scheduler.h>
#include <sign/Vms.h>
#include <sign/Islus.h>
#include <uci/DbHelper.h>
//#include <module/ptcpp.h>


Scheduler::Scheduler()
:unitedSigns(nullptr),groups(nullptr),tmrEvt(nullptr)
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
    if(unitedSigns != nullptr)
    {
        for(int i=0;i<signCnt;i++)
        {
            delete unitedSigns[i];
        }
        delete [] unitedSigns;
    }
    if(tmrEvt != nullptr)
    {
        tmrEvt->Remove(this);
    }
}

void Scheduler::Init(TimerEvent *tmrEvt_)
{
    if(tmrEvt!=nullptr)
    {
        MyThrow ( "Re-invoke Scheduler::Init()" );
    }
    tmrEvt=tmrEvt_;
    tmrEvt->Add(this);
    displayTimeout.Clear();

    // sign init
    signCnt=DbHelper::Instance().uciProd.NumberOfSigns();
    unitedSigns = new IUnitedSign * [signCnt];
    switch(DbHelper::Instance().uciProd.ProdType())
    {
        case 0:
            for(int i=0;i<signCnt;i++)
            {
                unitedSigns[i] = new Vms(i+1);
            }
        break;
        case 1:
            for(int i=0;i<signCnt;i++)
            {
                unitedSigns[i] = new Islus(i+1);
            }
        break;
    }

    // group init
    groupCnt=0;
    for(int i=1;i<=signCnt;i++)
    {
        uint8_t gid=DbHelper::Instance().uciUser.GetGroupIdOfSign(i);
        if(gid>groupCnt)
        {
            groupCnt=gid;
        }
    }
    groups = new Group * [groupCnt];
    for(int i=0;i<groupCnt;i++)
    {
        groups[i] = new Group(i+1);
    }
    for(int i=1;i<=signCnt;i++)
    {
        GetGroup(DbHelper::Instance().uciUser.GetGroupIdOfSign(i))->Add(GetUnitedSign(i));
    }
    for(int i=0;i<groupCnt;i++)
    {
        if(groups[i]->grpSigns.size()==0)
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

uint8_t Scheduler::CtrllerErr()
{
    return ctrllerErr;
}

