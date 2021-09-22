#include <cstring>
#include <sign/GroupVms.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>

GroupVms::GroupVms(uint8_t id)
    : Group(id)
{
}

GroupVms::~GroupVms()
{
}

void GroupVms::PeriodicHook()
{
    LoadDsNext();
    Run();
}

bool GroupVms::Run()
{
    PT_BEGIN();
    while (true)
    {
        if(dsCurrent->dispType == DISP_STATUS::TYPE::PLN)
        {
            
        }
    };
    PT_END();
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
