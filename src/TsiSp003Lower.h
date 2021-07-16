#ifndef __TSISP003LOWER_H__
#define __TSISP003LOWER_H__

#include <unistd.h>
#include "TsiSp003Cfg.h"
#include "TsiSp003App.h"
#include "BootTimer.h"
#include "IByteStream.h"
#include "DbHelper.h"
#include "ILowerLayer.h"
#include "TimerEvent.h"

#define  MAX_TsiSp003 16

/// \brief TsiSp003Lower is for all Lower layers of TsiSp003 management except for Application layer
/// \brief Including Session management, Start/End session, Password / Password Seed
class TsiSp003Lower : public ILowerLayer, public IPeriodicEvent
{
public:
    TsiSp003Lower();
    ~TsiSp003Lower();
    static TimerEvent * tmrEvent;

    /// \brief		check if there is a free space int tsiSp003s
    /// \brief      Must check before instantiate a new TsiSp003Lower
    /// \return     int : -1 failed
    int GetFreeSpace();

    /// \brief		periodic run
    void PeriodicRun();

    /// \brief		data received
    void Rx();

    /// \brief		send data
    void Tx();

private:
    /// \brief		byte stream for rx & tx
    IByteStream *byteStream;
    /// \brief		pointer to app layer interface
    TsiSp003App *tsiSp003App;
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
