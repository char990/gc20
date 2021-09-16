
#include <uci/UciEvent.h>

UciEvent::UciEvent()
{
    PATH = "./config";
    PACKAGE = "UciEvent";
	SECTION = "evt";
    pStrLog = new StrLog[EVENT_LOG_ENTRIES];
    maxEntries=EVENT_LOG_ENTRIES;
}

UciEvent::~UciEvent()
{
    delete [] pStrLog;
}
