#ifndef __PLAN_H__
#define __PLAN_H__

#include <cstdint>
#include <string>
#include <tsisp003/TsiSp003Const.h>

struct Hm
{
    uint8_t hour;
    uint8_t min;
};

struct PlnEntry
{
    uint8_t type;
    uint8_t fmId;
    struct Hm start;
    struct Hm stop;
};

class Plan
{
public:
    /// \breif  ini plan with string, with crc & enable
    Plan(char * cpln, int clen);
    
    /// \breif  ini plan with hex array, no CRC & enable
    Plan(uint8_t * xpln, int xlen);
    
    uint8_t appErr;
    uint8_t micode;
    uint8_t plnId;
    uint8_t plnRev;
    uint8_t weekdays;
    PlnEntry plnEntries[6];
    uint16_t crc;
    uint8_t enabled;
    uint8_t *plnData;
    int plnDataLen;
    

    std::string ToString();
};

#endif
