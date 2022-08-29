#pragma once

#include <module/IGcEvent.h>
#include <module/Utils.h>


#define DC_PRINT_BUF_SIZE PRINT_BUF_SIZE
class Command
{
public:
    const char * cmd;
    const char * help;
    void (*function)(int argc, char *argv[]);
};

class DebugConsole : public IGcEvent
{
public:
    DebugConsole();
    ~DebugConsole();
    virtual void EventsHandle(uint32_t events) override;
private:
    char inbuf[DC_PRINT_BUF_SIZE];
    int cnt{0};
    void Process();
    int _fcntl;

    // command list
    static void Cmd_help(int argc, char *argv[]);
    static void Cmd_t(int argc, char *argv[]);
    static void Cmd_ver(int argc, char *argv[]);
    static void Cmd_ws(int argc, char *argv[]);

    static const Command CMD_LIST[];
};
