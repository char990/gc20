#pragma once

#include <module/IGcEvent.h>

#define DC_INBUF_SIZE 256
class DebugConsole : public IGcEvent
{
public:
    DebugConsole();
    ~DebugConsole();
    virtual void EventsHandle(uint32_t events) override;
private:
    char inbuf[DC_INBUF_SIZE];
    int cnt{0};
    void Process();
    int _fcntl;

    // command list
    void Cmd_help();
    void Cmd_t();
    void Cmd_ver();
};
