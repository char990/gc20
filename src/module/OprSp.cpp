#include <unistd.h>
#include <module/MyDbg.h>
#include <module/OprSp.h>
#include <layer/UI_LayerManager.h>
#include <layer/SLV_LayerManager.h>
#include <module/Epoll.h>
#include <uci/DbHelper.h>

OprSp::OprSp(uint8_t comX, int bps, IUpperLayer *upperLayer)
    : OprSp(comX, bps, upperLayer, POWEROF2_MAX_DATA_PACKET_SIZE)
{
}

OprSp::OprSp(uint8_t comX, int bps, IUpperLayer *upperLayer, int rxbufsize)
    : IOperator(rxbufsize)
{
    this->comX = comX; 
    SpConfig &spCfg = gSpConfig[comX];
    spCfg.baudrate = bps;
    sp = new SerialPort(spCfg);
    this->upperLayer = upperLayer;
    if (upperLayer != nullptr)
    {
        upperLayer->LowerLayer(this);
        upperLayer->ClrRx();
    }
    if (sp->Open() < 0)
    {
        char buf[64];
        snprintf(buf, 63, "Open %s failed", sp->Config().name);
        DbHelper::Instance().GetUciAlarm().Push(0, buf);
        throw std::runtime_error(buf);
    }
    events = ((upperLayer != nullptr) ? EPOLLIN : 0) | EPOLLRDHUP;
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
    if (x > 0)
    {
        x = x * 1000 * sp->Config().bytebits / sp->Config().baudrate; // get ms
    }
    return (x < 10 ? 10 : x);
}

/// \brief  Called by Eepoll, receiving & sending handle
void OprSp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        char buf[64];
        snprintf(buf, 63, "%s closed: events=0x%08X", sp->Config().name, events);
        DbHelper::Instance().GetUciAlarm().Push(0, buf);
        if (ReOpen() == -1)
        {
            throw std::runtime_error(FmtException("%s closed: events=0x%08X and reopen failed", sp->Config().name, events));
        }
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
    if (n > 0)
    {
        if (IsTxRdy()) // if tx is busy, discard this rx
        {
            if (upperLayer != nullptr)
            {
                upperLayer->Rx(buf, n);
            }
        }
        else
        {
            PrintDbg(DBG_LOG, "%s:ComTx not ready", sp->Config().name);
        }
    }
    return 0;
}

int OprSp::ReOpen()
{
    DbHelper::Instance().GetUciAlarm().Push(0, "ReOpen %s", sp->Config().name);

    Epoll::Instance().DeleteEvent(this, events);
    sp->Close();
    eventFd = -1;
    if (sp->Open() < 0)
    {
        DbHelper::Instance().GetUciAlarm().Push(0, "ReOpen %s failed", sp->Config().name);
        return -1;
    }
    events = ((upperLayer != nullptr) ? EPOLLIN : 0) | EPOLLRDHUP;
    eventFd = sp->GetFd();
    Epoll::Instance().AddEvent(this, events);
    if (upperLayer != nullptr)
    {
        upperLayer->ClrRx();
    }
    ClrTx();
    return 0;
}
