#include <unistd.h>
#include <stdexcept>
#include <module/OprSp.h>
#include <layer/LayerManager.h>
#include <module/Epoll.h>

OprSp::OprSp(SerialPort & sp, std::string name_, std::string aType)
:sp(sp)
{
    name = name_;
    upperLayer = new LayerManager(name, aType);
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

/*< IOperator --------------------------------------------------*/
/// \brief  Called by Eepoll, receiving & sending handle
void OprSp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        printf("Disconnected:events=0x%08X\n", events);
        throw std::runtime_error(name + " closed");
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
        printf("disconnected\n");
        throw std::runtime_error(name + " read error");
    }
    else
    {
        //printf("%d bytes\n", n);
        //upperLayer->Rx(data, len);
    }
    return 0;
}