#ifndef __TSISP003_H__
#define __TSISP003_H__

#include <vector>
#include "TsiSp003Cfg.h"
#include "BootTimer.h"

#define  MAX_TsiSp003 16

class TsiSp003
{
public:
    TsiSp003();
    ~TsiSp003();

    /// \brief		space for all TsiSp003 objects
    static TsiSp003 * tsiSp003s[MAX_TsiSp003];
    
    /// \brief		check if there is a free space int tsiSp003s
    /// \brief      Must check before instantiate a new TsiSp003
    /// \return     int : -1 failed
    int GetFreeSpace();

    /// \brief		Set config
    /// \param		cfg		class pointer to config
    void SetCfg(TsiSp003Cfg *cfg);

    /// \brief		periodic run
    void Run();

    /// \brief		Check session timeout
    void SessionTimeout();

    /// \brief		Check display timeout
    void DisplayTimeout();

private:
    static TsiSp003Cfg *tsiSp003Cfg;

    BootTimer sessionTimeout;
    BootTimer displayTimeout;
};

#endif
