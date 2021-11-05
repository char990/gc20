#include <cstdio>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

UciAlarm::UciAlarm()
:UciStrLog(ALARM_LOG_ENTRIES)
{
}

UciAlarm::~UciAlarm()
{
}

void UciAlarm::LoadConfig()
{
    PrintDbg(DBG_LOG, ">>> Loading 'alarmlog'\n");
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciAlarm";
	SECTION = "alm";
    UciStrLog::LoadConfig();
}
