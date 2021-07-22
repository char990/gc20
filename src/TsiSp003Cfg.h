#ifndef __TSISP003CFG_H__
#define __TSISP003CFG_H__

#include <cstdint>
#include <string>

class TsiSp003Cfg
{
private:
    TsiSp003Cfg() {}
    std::string version; 

public:
    TsiSp003Cfg(TsiSp003Cfg const &) = delete;
    void operator=(TsiSp003Cfg const &) = delete;
    static TsiSp003Cfg &Instance()
    {
        static TsiSp003Cfg instance;
        return instance;
    }

    void Init(std::string ver);
};

#endif
