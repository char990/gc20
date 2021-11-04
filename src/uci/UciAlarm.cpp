#include <cstdio>
#include <uci/DbHelper.h>

UciAlarm::UciAlarm()
:UciStrLog(ALARM_LOG_ENTRIES)
{
}

UciAlarm::~UciAlarm()
{
}

void UciAlarm::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciAlarm";
	SECTION = "alm";
    UciStrLog::LoadConfig();
}
