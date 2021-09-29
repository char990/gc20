#include <cstring>
#include <uci/GrpProc.h>
#include <module/Utils.h>


using namespace Utils;

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

void GrpProc::ProcDisp(uint8_t *cmd, int len)
{
    if(len>255)len=0;
    disp[0]=len;
    memcpy(disp+1,cmd,len);
}
