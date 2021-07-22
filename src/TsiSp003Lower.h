#ifndef __TSISP003LOWER_H__
#define __TSISP003LOWER_H__

#include <unistd.h>
#include "TsiSp003Cfg.h"
#include "TsiSp003App.h"
#include "BootTimer.h"
#include "IByteStream.h"
#include "DbHelper.h"
#include "IAdaptLayer.h"
#include "TimerEvent.h"
#include "IOperator.h"


/// \brief TsiSp003Lower is for all Lower layers of TsiSp003 management except for Application layer
/// \brief Including Session management, Start/End session, Password / Password Seed
class TsiSp003Lower : public IAdaptLayer, public IPeriodicEvent
{
public:
    TsiSp003Lower(std::string name, IOperator * iOperator);
    ~TsiSp003Lower();
    static TimerEvent * tmrEvent;

    /// \brief		periodic run
    virtual void PeriodicRun()override;

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd) override;
    
    /// \brief Transmitting function, call Tx() of lowerLayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len) override;

private:
    std::string name;
    IOperator * iOperator;
    TsiSp003App app;

    /// \brief Session timeout timer
    BootTimer sessionTimeout;
    /// \brief Display timeout timer
    BootTimer displayTimeout;
    /// \brief		Check session timeout
    void SessionTimeout();

    /// \brief		Check display timeout
    void DisplayTimeout();

    /// \brief protocol fields 
    uint8_t nr, ns;
    void IncNr();
    void IncNs();
    uint16_t password;
    uint8_t seed;
};

#endif
