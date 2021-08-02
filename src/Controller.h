#ifndef __CONTORLLER_H__
#define __CONTORLLER_H__

#include <cstdint>
#include <string>
#include <vector>
#include "BootTimer.h"
#include "Sign.h"

class SignsInGroup
{
public:
    SignsInGroup(int i):
    :cnt(i)
    {
        signsInGrp = new Sign * [i];
    }
    ~SignsInGroup():
    {
        delete [] signsInGrp;
    }
    int cnt;
    Sign ** signsInGrp;
};

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

private:
    Controller();
    /// \brief Display timeout timer
    BootTimer displayTimeout;

    Sign * signs;

    SignsInGroup * groups;

    uint8_t
        signCnt,
        sessionLed,
        errorCode;
};

#endif
