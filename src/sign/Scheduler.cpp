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
        groups[groupcfg[i]]->Add(unitedSigns[i]);
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

