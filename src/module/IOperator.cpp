#include <unistd.h>
#include <cstring>
#include <module/IOperator.h>
#include <module/Epoll.h>

int IOperator::TxBytes(uint8_t *data, int len)
{
    if (ringBuf.Vacancy() < len || len <= 0 || eventFd < 0)
    {
        return -1;
    }
    if (ringBuf.Cnt() > 0 || txcnt < txsize)
    { // tx is busy
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
            txcnt = 0;
            int left = len - n;
            txsize = (left < OPTXBUF_SIZE) ? left : OPTXBUF_SIZE;
            left -= txsize;
            memcpy(optxbuf, data + n, txsize);
            if (left > 0)
            {
                ringBuf.Push(data + n + txsize, left);
            }
            events = ((upperLayer != nullptr) ? EPOLLIN : 0) | EPOLLOUT | EPOLLRDHUP;
            Epoll::Instance().ModifyEvent(this, events);
        }
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
        if (ringBuf.Cnt() == 0)
        {
            ClrTx();
            return 0;
        }
        else
        {
            txsize = ringBuf.Pop(optxbuf, OPTXBUF_SIZE);
            txcnt = 0;
        }
    }

    int len = txsize - txcnt;
    int n = write(eventFd, optxbuf + txcnt, len);
    if (n <= 0)
    {
        r = len;
    }
    else if (n <= len)
    {
        txcnt += n;
        r = txsize - txcnt;
    }
    return r;
}
