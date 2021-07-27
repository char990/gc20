#include <unistd.h>
#include <stdexcept>
#include "OprSp.h"
#include "LayerAdaptor.h"
#include "TimerEvent.h"

OprSp::OprSp(SerialPort *sp)
:sp(sp)
{
}

OprSp::~OprSp()
{
    if(upperLayer!=nullptr)
    {
        delete upperLayer;
        upperLayer=nullptr;
        sp->Close();
        if (eventFd > 0)
        {
            tmrEvent->Remove(this);
            Epoll::Instance().DeleteEvent(this, events);
            events = 0;
        }
    }
}

/* ILayer */
/// \brief  Called by upperLayer
int OprSp::Tx(uint8_t *data, int len)
{
    return TxEvt(data, len);
}

/// \brief  Called by upperLayer
void OprSp::Release()
{
}

/// \brief  Called by IOperator
int OprSp::Rx(uint8_t * data, int len)
{
    return upperLayer->Rx(data, len);
}

/// \brief  Called by IOperator
void OprSp::PeriodicRun()
{
    upperLayer->PeriodicRun();
}

/// \brief  Called by IOperator
void OprSp::Clean()
{
    upperLayer->Clean();
}

/* IOperator */
/// \brief  Called by Eepoll, receiving & sending handle
void OprSp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        printf("Disconnected:events=0x%08X\n", events);
        Release();
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

/// \brief  Called when instantiation
void OprSp::Init(std::string name_, std::string aType)
{
    name = name_ + ":OprSp";
    upperLayer = new LayerAdaptor(name, aType, this);
    sp->Open();
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = sp->GetFd();
    Epoll::Instance().AddEvent(this, events);
    tmrEvent->Add(this);
    Clean();
}

/// \brief  Called when a new connection accepted
void OprSp::Setup(int fd)
{
}

/// \brief  Called by ILayer.Tx()
int OprSp::TxEvt(uint8_t * data, int len)
{
    int x = TxBytes(data, len);
    if(x>0)
    {
        x = len * sp->Config()->bytebits / sp->Config()->baudrate;
    }
    return (x==0 ? 1 : x);
}

/// \brief  Called by TimerEvt
void OprSp::PeriodicEvt()
{
    PeriodicRun();
}

/// \brief  Called in EventsHandle
int OprSp::RxHandle()
{
    uint8_t buf[4096];
    int n = read(eventFd, buf, 4096);
    if (n <= 0)
    {
        printf("disconnected\n");
        Release();
    }
    else
    {
        Rx(buf, n);
        //printf("%d bytes\n", n);
    }
    return 0;
}

