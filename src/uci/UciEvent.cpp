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
    PrintDbg(DBG_LOG, ">>> Loading 'eventlog'\n");
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciEvent";
	SECTION = "evt";
    UciStrLog::LoadConfig();
}
