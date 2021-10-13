#pragma once


#include <uci/UciStrLog.h>

#define EVENT_LOG_ENTRIES 1000

/*
Filename: "./config/UciEvent"
Format: UCI

config UciEvent evt
*/

class UciEvent : public UciStrLog
{
public:
    UciEvent();
    ~UciEvent();

    virtual void LoadConfig() override;
private:

};


