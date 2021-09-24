#pragma once

#include <uci/UciLog.h>
#include <uci/StrLog.h>

/*
E.g:
Filename: "./config/UciEvent"
Format: UCI

The prefix of option should be same as section
E.g.
Section name is "evt", then option name is "evt_xxx" 
--- Start ---
config UciEvent evt
    # evt_xxx : xxx is event ID, 0-999
    // data format
    // uint8_t id
    // uint16_t entryNo
    // [local time string] : [31/12/2020 12:34:56]
    // string, max length is 63
    option evt_0  "............."

    // option LastLog is the last log entry number
    option LastLog "244"
--- End ---
*/

class UciStrLog : public UciLog
{
public:
    UciStrLog(){};
    virtual ~UciStrLog(){};

    virtual void LoadConfig() override;

    virtual int GetLog(uint8_t *dst);

    /// \brief  Push a string log into Log and save to Ucixxx
    virtual void Push(uint8_t id, const char *str);

protected:
    StrLog *pStrLog;
    int maxEntries;
};
