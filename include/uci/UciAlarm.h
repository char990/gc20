#pragma once


#include <uci/UciStrLog.h>

#define ALARM_LOG_ENTRIES 500

/*
Filename: "./config/UciAlarm"
Format: UCI

config UciAlarm alm
*/


class UciAlarm : public UciStrLog
{
public:
    UciAlarm();
    ~UciAlarm();

    virtual void LoadConfig() override;

private:
};

