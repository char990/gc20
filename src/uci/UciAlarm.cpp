#include <cstdio>
#include <uci/DbHelper.h>

UciAlarm::UciAlarm()
{
    pStrLog = new StrLog[ALARM_LOG_ENTRIES];
    maxEntries=ALARM_LOG_ENTRIES;
}

UciAlarm::~UciAlarm()
{
    delete [] pStrLog;
}

void UciAlarm::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciAlarm";
	SECTION = "alm";
    UciStrLog::LoadConfig();
}
