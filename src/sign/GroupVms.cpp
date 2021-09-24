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


void GroupVms::Add(Sign *sign)
{
    if(signCnt==0)
    {
        vSigns.push_back(sign);
        sign->Reset();
        signCnt++;
        // init slaves
    }
    else
    {
        MyThrow ("VMS: Only ONE Sign in a group");
    }
}

void GroupVms::PeriodicHook()
{
}

