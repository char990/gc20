#ifndef __TSISP003CFG_H__
#define __TSISP003CFG_H__

#include <cstdint>
#include <string>

class TsiSp003Cfg
{
public:
    TsiSp003Cfg(std::string ver);
private:
    const std::string version; 
};

#endif
