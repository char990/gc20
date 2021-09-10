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


#define PLN_LEN_MAX (4+6*6)
#define PLN_LEN_MIN (4+1*6+1)
#define PLN_ENABLED 0x55
#define PLN_DISABLED 0xAA

class Plan
{
public:
    /// \breif  Blank plan
    Plan():micode(0),crc(0){};
    ~Plan(){};

    /// \breif  ini plan with string, with crc & enable
    ///         check crc & set enabled
    APP::ERROR Init(char * cpln, int clen);
    
    /// \breif  ini plan with hex array, no CRC & enable
    ///         crc wille be regenerated, but do nothing with enabled
    APP::ERROR Init(uint8_t * xpln, int xlen);
    
    uint8_t micode;
    uint8_t plnId;
    uint8_t plnRev;
    uint8_t weekdays;
    PlnEntry plnEntries[6];
    uint16_t crc;
    uint8_t enabled;

    ///\brief convert this plan to uint8_t array in format "Sign Set Plan"
    /// \param  pbuf : output buf
    /// \return int : length of data
    int ToArray(uint8_t * pbuf);

    // for Dump()
    std::string ToString();
};

#endif
