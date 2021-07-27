#ifndef __TCPOPERATOR_H__
#define __TCPOPERATOR_H__

#include "IOperator.h"
#include "ILayer.h"
#include "BootTimer.h"



class TcpServer;

/// \brief  Operator from tcp
class OprTcp : public IOperator, public ILayer
{
public:
    OprTcp();
    ~OprTcp();

    /* ILayer */
    /// \brief  Called by upperLayer
    virtual int Tx(uint8_t * data, int len) override;
    /// \brief  Called by upperLayer
    virtual void Release() override;
    /// \brief  Called by IOperator
    virtual int Rx(uint8_t * data, int len) override;
    /// \brief  Called by IOperator
    virtual void PeriodicRun() override;
    /// \brief  Called by IOperator
    virtual void Clean() override;

    /* IOperator */
    /// \brief  Called by Eepoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /// \brief  Called when instantiation
    virtual void Init(std::string name, std::string aType) override;
    /// \brief  Called when a new connection accepted
    virtual void Setup(int fd) override;
    /// \brief  Called by ILayer.Tx()
    virtual int TxEvt(uint8_t * data, int len) override;
    /// \brief  Called by TimerEvt
    virtual void PeriodicEvt() override;

    /// \brief  Called in EventsHandle
    int RxHandle();

    /// \brief      Set Tcpserver for Release()
    void SetServer(TcpServer * server);

    /// \brief      Set Tcp Idle timeout
    /// \param      idleTime: seconds
    void IdleTime(int idleTime);

    std::string Name() { return name; };
private:
    std::string name;
    TcpServer * server;
    BootTimer tcpIdleTmr;
    int idleTime;
};

#endif
