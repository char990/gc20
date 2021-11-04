#include <cstring>
#include <uci/GrpProc.h>
#include <module/Utils.h>


using namespace Utils;

bool GrpProc::IsPlanEnabled(uint8_t id)
{
    return (id == 0) ? false : enabledPln.Get(id);
}

void GrpProc::EnDisPlan(uint8_t id, bool endis)
{
    (id == 0) ? enabledPln.ClrAll() :
                (endis ? enabledPln.Set(id) : enabledPln.Clr(id));
}

void GrpProc::ProcDisp(uint8_t *cmd, int len)
{
    if(len>255)len=0;
    disp[0]=len;
    if(len>0)
    {
        memcpy(disp+1,cmd,len);
    }
}
