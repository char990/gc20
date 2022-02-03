#include <unistd.h>
#include <cstring>
#include <module/IOperator.h>
#include <module/Epoll.h>

int IOperator::TxBytes(uint8_t *data, int len)
{
    if (ringBuf.Cap() < len || len <= 0 || eventFd < 0)
    {
        return -1;
    }
    if(ringBuf.Cnt()>0 || txcnt<txsize)
    {// tx is busy
        ringBuf.Push(data, len);
    }
    else
    {
        int n = write(eventFd, data, len);
        if (n < 0)
        {
            return -1;
        }
        else if (n < len)
        {
            ringBuf.Push(data+n, len-n);
            events = ((upperLayer != nullptr) ? EPOLLIN : 0) | EPOLLOUT | EPOLLRDHUP;
            Epoll::Instance().ModifyEvent(this, events);
        }
    }
    return len;
    if (txsize != 0 || len <= 0 || len > bufsize || eventFd < 0)
    {
        return -1;
    }
    int n = write(eventFd, data, len);
    if (n < 0)
    {
        return -1;
    }
    else if (n < len)
    {
        txsize = len - n;
        txcnt = 0;
        memcpy(optxbuf, data + n, txsize);
        events = ((upperLayer != nullptr) ? EPOLLIN : 0) | EPOLLOUT | EPOLLRDHUP;
        Epoll::Instance().ModifyEvent(this, events);
    }
    return len;
}

void IOperator::ClrTx()
{
    txsize = 0;
    txcnt = 0;
    ringBuf.Reset();
    events = ((upperLayer != nullptr) ? EPOLLIN : 0) | EPOLLRDHUP;
    Epoll::Instance().ModifyEvent(this, events);
}

int IOperator::TxHandle()
{
    int r = 0;
    if (txcnt == txsize)
    {
        if(ringBuf.Cnt()==0)
        {
            ClrTx();
            return 0;
        }
        else
        {
            ringBuf.Pop(optxbuf, 4096);
            txsize = 4096;
            txcnt = 0;
        }
    }

    int len = txsize - txcnt;
    int n = write(eventFd, optxbuf + txcnt, len);
    if (n < 0)
    {
        ClrTx();
        r = -1;
    }
    else if (n < len)
    {
        txcnt += n;
        r = txsize - txcnt;
    }
    return r;

    if (txcnt == txsize)
    {
        ClrTx();
    }
    else
    {
        int len = txsize - txcnt;
        int n = write(eventFd, optxbuf + txcnt, len);
        if (n < 0)
        {
            ClrTx();
            r = -1;
        }
        else if (n < len)
        {
            txcnt += n;
            r = txsize - txcnt;
        }
        else
        {
            ClrTx();
        }
    }
    return r;
}
