#pragma once


#include <uci/UciStrLog.h>

#define EVENT_LOG_ENTRIES 1000

/*
Filename: "./config/UciEvent"
Format: UCI
config UciEvent evt
    # evt_xxx : xxx is option ID, 0-999
    // data format: string, max length is 63
    option evt_0  "............."

    // option LastLog is the last log entry number
    option LastLog "244"
*/

class UciEvent : public UciStrLog
{
public:
    UciEvent();
    ~UciEvent();

    virtual void LoadConfig() override;
private:

};


