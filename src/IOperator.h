#ifndef __IOPERATOR_H__
#define __IOPERATOR_H__

#include <string>
#include "IGcEvent.h"
#include "TimerEvent.h"

class IOperator : public IGcEvent , public IPeriodicEvent
{
public:
    virtual void EventsHandle(uint32_t events)=0;

    /// \brief Transmitting function, called by upper Tx() and call lower Tx() 
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int TxEvt(uint8_t * data, int len)=0;

    /// \brief  Called only after object was created
    virtual void Init(std::string aType, std::string name)=0;

    /// \brief  Called when connection was accepted
    virtual void Setup(int fd)=0;

    /// \brief  PeriodicEvt
    virtual void PeriodicEvt()=0;

    /// \brief  Registation of timer for epoll
    static TimerEvent * tmrEvent;

    int TxBytes(uint8_t * data, int len);
    int TxHandle();
    void ClrTx();
    
    static const int OPR_TX_BUF_SIZE=129*1024;
    uint8_t txbuf[OPR_TX_BUF_SIZE];
    int txsize;
    int txcnt;
};

#endif
