#ifndef __CONTORLLER_H__
#define __CONTORLLER_H__

#include <cstdint>
#include <string>
#include <list>
#include <module/BootTimer.h>
#include <sign/Sign.h>

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
    ~Controller();

    void Init();

    void RefreshDispTime();

    void SessionLed(uint8_t v);

    uint8_t ErrorCode();
    uint8_t SignCnt();
    Sign ** signs;

private:
    Controller();
    /// \brief Display timeout timer
    BootTimer displayTimeout;

    uint8_t
        signCnt,
        sessionLed,
        errorCode;
};

#endif
