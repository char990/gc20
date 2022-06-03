#include <uci/DbHelper.h>
#include <module/MyDbg.h>

UciEvent::UciEvent()
:UciStrLog(EVENT_LOG_ENTRIES)
{
}

UciEvent::~UciEvent()
{
}

void UciEvent::LoadConfig()
{
    Ldebug(">>> Loading 'eventlog'");
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciEvent";
	SECTION = "evt";
    UciStrLog::LoadConfig();
}
