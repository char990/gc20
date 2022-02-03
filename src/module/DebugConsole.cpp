#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <module/DebugConsole.h>
#include <module/Epoll.h>

DebugConsole::DebugConsole()
{
    // set stdin
    _fcntl = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, _fcntl | O_NONBLOCK);
    eventFd = 0;
    events = EPOLLIN;
    Epoll::Instance().AddEvent(this, events);
}

DebugConsole::~DebugConsole()
{
    Epoll::Instance().DeleteEvent(this, events);
    fcntl(0, F_SETFL, _fcntl);
}

void DebugConsole::EventsHandle(uint32_t events)
{
    if (events & EPOLLIN)
    {
        while (1)
        {
            char *p = inbuf + cnt;
            int numRead = read(0, p, DC_INBUF_SIZE - cnt - 1);
            if (numRead <= 0)
            {
                return;
            }
            cnt += numRead;
            if (cnt == DC_INBUF_SIZE - 1)
            {
                cnt = 0;
            }
            else
            {
                char *pe = p + numRead;
                while (p < pe)
                {
                    if (*p == '\n')
                    {
                        *p = '\0';
                        Process();
                        cnt = 0; // clear inbuf
                    }
                    p++;
                }
            }
        }
    }
}

void DebugConsole::Process()
{
    if (strcmp(inbuf, "t") == 0)
    {
        Cmd_t();
    }
    else if (strcmp(inbuf, "?") == 0)
    {
        Cmd_help();
    }
    else if (strcmp(inbuf, "ver") == 0)
    {
        Cmd_ver();
    }
    else
    {
        int len = strlen(inbuf);
        if (len > 0 && len < DC_INBUF_SIZE)
        {
            printf("Unknown command\nPlease use command from the Command list:\n");
            Cmd_help();
        }
    }
    printf("\n=>");
    fflush(stdout);
}

void DebugConsole::Cmd_help()
{
    PrintDash('-');
    printf("CMD\t| Comments\n");
    PrintDash('-');
    printf(
        "?       | This help\n"
        "t       | Ticktock ON/OFF\n"
        "ver     | Version\n");
    PrintDash('-');
}

extern bool ticktock;
void DebugConsole::Cmd_t()
{
    printf("Ticktock %s\n", ticktock ? "OFF" : "ON");
    ticktock = !ticktock;
}

extern void PrintVersion();
void DebugConsole::Cmd_ver()
{
    PrintVersion();
}
