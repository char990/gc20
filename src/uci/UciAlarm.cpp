#include <cstdio>
#include <uci/UciAlarm.h>

UciAlarm::UciAlarm()
{
    PATH = "./config";
    PACKAGE = "UciAlarm";
	SECTION = "alm";
    pStrLog = new StrLog[ALARM_LOG_ENTRIES];
    maxEntries=ALARM_LOG_ENTRIES;
}

UciAlarm::~UciAlarm()
{
    delete [] pStrLog;
}
