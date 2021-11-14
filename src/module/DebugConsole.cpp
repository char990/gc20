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

extern bool ticktock;
void DebugConsole::Process()
{
    printf("Console got '%s'\n", inbuf);
    if(strcmp(inbuf,"t")==0)
    {
        ticktock=!ticktock;
    }
}
