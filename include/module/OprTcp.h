#pragma once
#include <string>

#include <module/BootTimer.h>
#include <module/IOperator.h>
#include <module/IPeriodicRun.h>
#include <module/TimerEvent.h>

#include <module/ObjectPool.h>

enum class TcpSvrType
{
    TMC
};

extern std::string TcpSvrTypeName(TcpSvrType t);

/// \brief  Operator from tcp
class OprTcp : public IOperator, public IPeriodicRun, public Poolable<OprTcp>
{
public:
    OprTcp();
    ~OprTcp();

    /*< IOperator ----------------------------------------------*/
    /// \brief  Called by upperLayer
    virtual bool IsTxReady() override { return IsTxRdy(); };
    /// \brief  Called by upperLayer
    virtual int Tx(uint8_t *data, int len) override;

    /// \brief  Called by Eepoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /*--------------------------------------------------------->*/

    /*< IPeriodicRun -------------------------------------------*/
    /// \brief  Called by TimerEvt
    virtual void PeriodicRun() override;
    /*--------------------------------------------------------->*/

    /*< For ObjectPool Pop/Push --------------------------------*/
    virtual void PopClean() override;
    virtual void PushClean() override;
    /*--------------------------------------------------------->*/

    /// \brief  Called only after object was created
    virtual void Init(int id, TcpSvrType serverType);

    /// \brief  Called when accept
    virtual void Accept(int fd, TimerEvent *tmr, const char *ip);

    std::string Name() { return name; };

    void Release();

private:
    std::string name;
    TimerEvent *tmrEvt{nullptr};
    BootTimer tcpIdleTmr;
    long IdleMs();

    /// \brief  Called in EventsHandle
    int RxHandle();

    char client[24];
};
