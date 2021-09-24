#include <uci/GrpProc.h>
#include <module/Utils.h>

using namespace Utils;

GrpProc::GrpProc()
{
    for (int i = 0; i < 255; i++)
    {
        enabledPln[i] = 0;
    }
    procDisp.dispType = DISP_STATUS::TYPE::FRM;
    procDisp.fmpLen = 1;
    procDisp.fmpid[0] = 0;
    dimming = 0;
}

bool GrpProc::IsPlanEnabled(uint8_t id)
{
    if (id == 0)
        return false;
    return (enabledPln[id - 1] != 0);
}

void GrpProc::EnablePlan(uint8_t id)
{
    if (id == 0)
    {
        DisablePlan(0);
    }
    else
    {
        enabledPln[id - 1] = 1;
    }
}

void GrpProc::DisablePlan(uint8_t id)
{
    if (id == 0)
    {
        for (int i = 0; i < 255; i++)
        {
            enabledPln[i] = 0;
        }
    }
    else
    {
        enabledPln[id - 1] = 0;
    }
}
