#include "module/Monitor.h"
#include <unistd.h>
#include <module/MyDbg.h>
#include <module/Epoll.h>

Monitor::Monitor(uint8_t comX, int bps)
{
    SpConfig & spCfg = gSpConfig[comX];
    spCfg.baudrate = bps;
    sp = new SerialPort(spCfg);
    if(sp->Open()<0)
    {
        char buf[64];
        snprintf(buf, 63, "Open %s failed", sp->Config().name);
        MyThrow (buf);
    }
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = sp->GetFd();
    Epoll::Instance().AddEvent(this, events);
}

void Monitor::TX(uint8_t buf, int len)
{/*
    if(txsize!=0 || len <=0 || len>MAX_DATA_PACKET_SIZE || eventFd<0)
    {
        return -1;
    }
    int n = write(eventFd, data, len);
    if(n<0)
    {
        return -1;
    }
    else if(n<len)
    {
        txsize=len-n;
        txcnt=0;
        memcpy(optxbuf, data+n, txsize);
        events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
        Epoll::Instance().ModifyEvent(this, events);
    }
    return len;
    */
}

void Monitor::EventsHandle(uint32_t events)
{
    if (events & EPOLLOUT)
    {
        TxHandle();
    }
    else
    {
        UnknownEvents(sp->Config().name, events);
    }
}

int Monitor::TxHandle()
{

}