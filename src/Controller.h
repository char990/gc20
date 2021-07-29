#ifndef __CONTORLLER_H__
#define __CONTORLLER_H__

#include <cstdint>
#include <string>


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



private:
    Controller() {}
};

#endif
