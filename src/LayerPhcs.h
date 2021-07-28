#ifndef __PHCS2APPADAPTOR_H__
#define __PHCS2APPADAPTOR_H__

#include <unistd.h>
#include <string>

#include "BootTimer.h"
#include "DbHelper.h"
#include "TimerEvent.h"

#include "ILayer.h"
#include "IOperator.h"


/// \brief  LayerPhcs is for all Lower layers of TsiSp003 management except for presentation & application layer
///         Including Session management ///???, Start/End session, Password / Password Seed
///         For ILayer in LayerPhcs: Lower Layer is no used, Upper Layer is TsiSp003Prst
///         Lower level read & write are in iOperator
class LayerPhcs : public ILayer
{
public:
    LayerPhcs(std::string name, bool & online);
    ~LayerPhcs();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void PeriodicRun() override;

    void Clean() override;

    void Release() override;

private:
    std::string name;

    /// \brief Session timeout timer
    BootTimer sessionTimeout;
    /// \brief Display timeout timer
    BootTimer displayTimeout;
    /// \brief		Check session timeout
    void SessionTimeout();

    /// \brief		Check display timeout
    void DisplayTimeout();

    /// \brief protocol fields 
    bool & online;
    uint8_t nr, ns;
    void IncNr();
    void IncNs();
    uint16_t password;
    uint8_t seed;

    int rcvd;

};

#endif
