#include <unistd.h>
#include <cstring>
#include "IOperator.h"

TimerEvent * IOperator::tmrEvent;

int IOperator::TxBytes(uint8_t * data, int len)
{
    if(txsize!=0 || len <=0 || len>OPR_TX_BUF_SIZE)
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
        memcpy(data+n, txbuf, txsize);
        events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
        Epoll::Instance().ModifyEvent(this, events);
    }
    return len;
}

extern void PrintTime();
void IOperator::ClrTx()
{
    txsize=0;
    txcnt=0;
    events = EPOLLIN | EPOLLRDHUP;
    Epoll::Instance().ModifyEvent(this, events);
    PrintTime();
}

int IOperator::TxHandle()
{
    int r=0;
    if(txcnt==txsize)
    {
        ClrTx();
    }
    else
    {
        int len = txsize-txcnt;
        int n = write(eventFd, txbuf+txcnt, len);
        if(n<0)
        {
            ClrTx();
            r = -1;
        }
        else if(n<len)
        {
            txcnt+=n;
            r=txsize-txcnt;
        }
        else
        {
            ClrTx();
        }
    }
    return r;
}
