#include <uci/DbHelper.h>

UciEvent::UciEvent()
{
    pStrLog = new StrLog[EVENT_LOG_ENTRIES];
    maxEntries=EVENT_LOG_ENTRIES;
}

UciEvent::~UciEvent()
{
    delete [] pStrLog;
}

void UciEvent::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciEvent";
	SECTION = "evt";
    UciStrLog::LoadConfig();
}
