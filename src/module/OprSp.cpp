#include <unistd.h>
#include <module/MyDbg.h>
#include <module/OprSp.h>
#include <layer/UI_LayerManager.h>
#include <layer/SLV_LayerManager.h>
#include <module/Epoll.h>

OprSp::OprSp(SerialPort & sp, std::string name_, std::string aType)
:sp(sp)
{
    name = name_;
    if(aType.compare("SLV")==0)
    {
        upperLayer = new SLV_LayerManager(name, aType);
    }
    else
    {
        upperLayer = new UI_LayerManager(name, aType);
    }
    upperLayer->LowerLayer(this);
    upperLayer->Clean();
    sp.Open();
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = sp.GetFd();
    Epoll::Instance().AddEvent(this, events);
}

OprSp::~OprSp()
{
    delete upperLayer;
    Epoll::Instance().DeleteEvent(this, events);
    sp.Close();
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
        x = x * sp.Config().bytebits / sp.Config().baudrate;
    }
    return (x==0 ? 1 : x);
}

/// \brief  Called by Eepoll, receiving & sending handle
void OprSp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        MyThrow ("%s closed: events=0x%08X\n", name, events);
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
        UnknownEvents(name, events);
    }
}

/// \brief  Called in EventsHandle
int OprSp::RxHandle()
{
    uint8_t buf[4096];
    int n = read(eventFd, buf, 4096);
    if (n <= 0)
    {
        MyThrow ("%s: read error", name);
    }
    else
    {
        printf("ComRx %d bytes\n", n);
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
