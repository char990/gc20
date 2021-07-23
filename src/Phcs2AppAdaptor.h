#ifndef __PHCS2APPADAPTOR_H__
#define __PHCS2APPADAPTOR_H__

#include <unistd.h>
#include "TsiSp003Cfg.h"
#include "TsiSp003App.h"
#include "BootTimer.h"
#include "DbHelper.h"
#include "IAppAdaptor.h"
#include "TimerEvent.h"
#include "IOperator.h"


/// \brief Phcs2AppAdaptor is for all Lower layers of TsiSp003 management except for Application layer
/// \brief Including Session management, Start/End session, Password / Password Seed
class Phcs2AppAdaptor : public IAppAdaptor
{
public:
    Phcs2AppAdaptor(std::string name, IOperator * iOperator);
    ~Phcs2AppAdaptor();

    /// \brief		periodic run
    virtual void PeriodicRun()override;

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd) override;
    
    /// \brief Transmitting function, call Tx() of adaptor
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len) override;

private:
    std::string name;
    IOperator * iOperator;
    TsiSp003App *app;

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
