#ifndef __TCPOPERATOR_H__
#define __TCPOPERATOR_H__

#include "BootTimer.h"
#include "IOperator.h"
#include "IPeriodicRun.h"

class TcpServer;

/// \brief  Operator from tcp
class OprTcp : public IOperator, public IPeriodicRun
{
public:
    OprTcp();
    ~OprTcp();

    /*< IOperator --------------------------------------------------*/
    /// \brief  Called by upperLayer
    virtual int Tx(uint8_t * data, int len) override;
    /// \brief  Called by Eepoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /*--------------------------------------------------------->*/

    /*< IPeriodicRun --------------------------------------------------*/
    /// \brief  Called by TimerEvt
    virtual void PeriodicRun() override;
    /*--------------------------------------------------------->*/

    /// \brief  Called only after object was created
    virtual void Init(std::string name_, std::string aType, int idle);

    /// \brief  Called when accept
    virtual void Setup(int fd);

    /// \brief      Set Tcpserver for Release()
    void SetServer(TcpServer * server);

    /// \brief      Set Tcp Idle timeout
    /// \param      idleTime: seconds
    void IdleTime(int idleTime);

    void Release();

    std::string Name() { return name; };

private:
    std::string name;
    TcpServer * server;
    BootTimer tcpIdleTmr;
    int idleTime;

    /// \brief  Called in EventsHandle
    int RxHandle();
};

#endif