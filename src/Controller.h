#ifndef __CONTORLLER_H__
#define __CONTORLLER_H__

#include <cstdint>
#include <string>
#include "BootTimer.h"


class Controller
{
public:
    Controller(Controller const &) = delete;
    void operator=(Controller const &) = delete;
    static Controller &Instance()
    {
        static Controller instance;
        return instance;
    }

    void Init();

    void RefreshDispTime();

    void SessionLed(uint8_t v);


private:
    Controller(){};
    /// \brief Display timeout timer
    BootTimer displayTimeout;
    uint8_t sessionLed;

};

#endif
