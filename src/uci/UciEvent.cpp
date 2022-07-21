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
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciEvent";
	SECTION = "evt";
    Ldebug(">>> Loading '%s/%s'", PATH, PACKAGE);
    UciStrLog::LoadConfig();
}
