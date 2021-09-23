#include <cstring>
#include <sign/GroupVms.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>

GroupVms::GroupVms(uint8_t id)
    : Group(id), taskPlnLine(0), taskMsgLine(0), msgEnd(1)
{
}

GroupVms::~GroupVms()
{
}

void GroupVms::PeriodicHook()
{
    bool reload = false;
    if(IsDsNextEmergency())
    {
        msgEnd=1;
    }
    if(msgEnd)
    {
        reload = LoadDsNext();
    }

    TaskPln(&taskPlnLine);
    TaskMsg(&taskMsgLine);
}

bool GroupVms::TaskPln(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        printf("TaskPln Step 0, delay 1 sec\n");
        task1Tmr.Setms(1000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
        printf("TaskPln Step 1, delay 3 sec\n");
        task1Tmr.Setms(3000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
        printf("TaskPln Step 2, delay 5 sec\n");
        task1Tmr.Setms(5000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());

        if(dsCurrent->dispType == DISP_STATUS::TYPE::PLN)
        {
            
        }
    };
    PT_END();
}

bool GroupVms::TaskMsg(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        printf("TaskMsg, every 2 sec\n");
        task2Tmr.Setms(2000);
        PT_WAIT_UNTIL(task2Tmr.IsExpired());
    };
    PT_END();
}

bool GroupVms::IsDsNextEmergency()
{
    bool r=false;
    if (dsExt->dispType == DISP_STATUS::TYPE::EXT)
    {// external input
        uint8_t mid = dsExt->fmpid[0];
        if(mid>=3 && mid<=5)
        {
            if(DbHelper::Instance().uciUser.ExtSwCfgX(mid)->emergency)
            {
                r=true;
            }
        }
    }
    return r;
}

bool GroupVms::LoadDsNext()
{
    bool r=false;
    if (dsNext->dispType == DISP_STATUS::TYPE::FSW)
    {// facility switch
        dsBak->Frm0();
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r=true;
    }
    else if (dsExt->dispType == DISP_STATUS::TYPE::EXT)
    {// external input
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsExt);
        dsExt->N_A();
        r=true;
    }
    else if (dsNext->dispType != DISP_STATUS::TYPE::N_A)
    {// display command
        dsBak->Clone(dsCurrent);
        dsCurrent->Clone(dsNext);
        dsNext->N_A();
        r=true;
    }
    if (dsCurrent->dispType == DISP_STATUS::TYPE::N_A)
    { // if current == N/A, load frm[0] to activate plan
        dsBak->Frm0();
        dsCurrent->Frm0();
        r=true;
    }
    if (dsCurrent->fmpid[0] == 0 &&
        (dsCurrent->dispType == DISP_STATUS::TYPE::FRM || dsCurrent->dispType == DISP_STATUS::TYPE::MSG))
    {//frm[0] or msg[0], activate plan
        dsCurrent->dispType = DISP_STATUS::TYPE::PLN;
        r=true;
    }
    return r;
}

/*
    uint8_t signId, power;
    uint8_t dimmingSet, dimmingV;
    uint8_t deviceSet, deviceV;

    DispStatus dsBak;
    DispStatus dsCurrent;
    DispStatus dsNext;
    DispStatus dsExt;
        uint8_t currentPln, currentMsg, currentFrm;
*/
