#include <uci/DbHelper.h>

UciEvent::UciEvent()
:UciStrLog(EVENT_LOG_ENTRIES)
{
}

UciEvent::~UciEvent()
{
}

void UciEvent::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciEvent";
	SECTION = "evt";
    UciStrLog::LoadConfig();
}
