#ifndef __WEB2APPLOWER_H__
#define __WEB2APPLOWER_H__

#include "ILowerLayer.h"

class Web2AppLower : public ILowerLayer
{
    public:
    /// \brief		periodic run
    void PeriodicRun();

    /// \brief		data received
    void Rx();

    /// \brief		send data
    void Tx();
};

#endif
