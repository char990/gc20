#ifndef __TSISP003App_H__
#define __TSISP003App_H__

#include "TsiSp003Cfg.h"

/// \brief TsiSp003App is only for TsiSp003 Application layer
/// \brief No session management
/// \brief No Start/End session, Password / Password Seed
class TsiSp003App
{
public:
    TsiSp003App();
    ~TsiSp003App();

    virtual std::string Version()=0;

    /// \brief Receiving Handle.
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: unknown command
    virtual int Received(uint8_t * data, int len);

protected:

private:

};

#endif
