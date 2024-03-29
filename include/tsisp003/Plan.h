#pragma once


#include <cstdint>
#include <string>
#include <tsisp003/TsiSp003Const.h>

struct Hm
{
    uint8_t hour;
    uint8_t min;
    std::string ToString();
};

enum PLN_ENTRY_TYPE{
    PLN_ENTRY_NA = 0,
    PLN_ENTRY_FRM = 1,
    PLN_ENTRY_MSG = 2
};
struct PlnEntry
{
    uint8_t fmType{0};
    uint8_t fmId;
    struct Hm start;
    struct Hm stop;
    std::string ToString();
};


#define PLN_LEN_MAX (4+6*6)
#define PLN_LEN_MIN (4+1*6+1)
#define PLN_TAIL 2

class Plan
{
public:
    /// \breif  Blank plan
    Plan(){};
    ~Plan(){};

    /// \breif  ini plan with hex array, with 2-byte crc
    APP::ERROR Init(uint8_t * xpln, int xlen);
    
    uint8_t micode{0};
    uint8_t plnId;
    uint8_t plnRev;
    uint8_t weekdays;
    uint8_t entries;
    PlnEntry plnEntries[6];
    uint16_t crc{0};

    ///\brief convert this plan to uint8_t array in format "Sign Set Plan" + with 2-byte crc
    /// \param  pbuf : output buf
    /// \return int : length of data
    int ToArray(uint8_t * pbuf);

    // for Dump()
    std::string ToString();

private:
    int CheckEntries();

};

