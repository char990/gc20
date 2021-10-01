#include <unistd.h>
#include <module/MyDbg.h>
#include <module/OprSp.h>
#include <layer/UI_LayerManager.h>
#include <layer/SLV_LayerManager.h>
#include <module/Epoll.h>

OprSp::OprSp(uint8_t comX, int bps, IUpperLayer * upperLayer)
:comX(comX)
{
    SpConfig & spCfg = gSpConfig[comX];
    spCfg.baudrate = bps;
    sp = new SerialPort(spCfg);
    this->upperLayer = upperLayer;
    upperLayer->LowerLayer(this);
    upperLayer->ClrRx();
    sp->Open();
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = sp->GetFd();
    Epoll::Instance().AddEvent(this, events);
}

OprSp::~OprSp()
{
    Epoll::Instance().DeleteEvent(this, events);
    sp->Close();
    delete sp;
}

/*< IOperator --------------------------------------------------*/

/// \brief  Called by upperLayer
bool OprSp::IsTxReady()
{
    return IsTxRdy();
}

/// \brief  Called by upperLayer
int OprSp::Tx(uint8_t *data, int len)
{
    int x = TxBytes(data, len);
    if(x>0)
    {
        x = x * 1000 * sp->Config().bytebits / sp->Config().baudrate; // get ms
    }
    return (x<10 ? 10 : x);
}

/// \brief  Called by Eepoll, receiving & sending handle
void OprSp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        MyThrow ("%s closed: events=0x%08X\n", sp->Config().name, events);
    }
    else if (events & EPOLLIN)
    {
        RxHandle();
    }
    else if (events & EPOLLOUT)
    {
        TxHandle();
    }
    else
    {
        UnknownEvents(sp->Config().name, events);
    }
}

/// \brief  Called in EventsHandle
int OprSp::RxHandle()
{
    uint8_t buf[4096];
    int n = read(eventFd, buf, 4096);
    if (n <= 0)
    {
        MyThrow ("%s: read error", sp->Config().name);
    }
    else
    {
        //printf("ComRx %d bytes\n", n);
        if(IsTxRdy()) // if tx is busy, discard this rx
        {
            upperLayer->Rx(buf, n);
        }
        else
        {
            printf("ComTx not ready\n");
        }
    }
    return 0;
}
