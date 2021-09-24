#include <cstring>
#include <sign/GroupIslus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>

GroupIslus::GroupIslus(uint8_t id)
:Group(id)
{
}

GroupIslus::~GroupIslus()
{
}

void GroupIslus::PeriodicHook()
{
}

void GroupIslus::Add(Sign *sign)
{
    vSigns.push_back(sign);
    sign->Reset();
    signCnt++;
    // slaves
}
