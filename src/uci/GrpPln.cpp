#include <uci/GrpPln.h>
#include <module/Utils.h>

using namespace Utils;

GrpPln::GrpPln()
{
    for (int i = 0; i < 255; i++)
    {
        enabledPln[i] = 0;
    }
}

bool GrpPln::IsPlanEnabled(uint8_t id)
{
    if(id==0) return false;
    return (enabledPln[id-1] != 0 );
}

void GrpPln::EnablePlan(uint8_t id)
{
    if(id==0)
    {
        return;
    }
    enabledPln[id-1] = 1;
}

void GrpPln::DisablePlan(uint8_t id)
{
    if(id==0)
    {
        for(int i=0;i<255;i++)
        {
            enabledPln[i]=0;
        }
    }
    else
    {
        enabledPln[id-1] = 0;
    }
}
