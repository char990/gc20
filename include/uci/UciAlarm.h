#pragma once


#include <uci/UciStrLog.h>

#define ALARM_LOG_ENTRIES 500

/*
Filename: "./config/UciAlarm"
Format: UCI
config UciAlarm alm
    # alm_xxx : xxx is option ID, 0-499
    // data format: string, max length is 63
    option alm_0  "............."

    // option LastLog is the last log entry number
    option LastLog "244"
*/

class UciAlarm : public UciStrLog
{
public:
    UciAlarm();
    ~UciAlarm();

private:
};

