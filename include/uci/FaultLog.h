#ifndef __FAULTLOG_H__
#define __FAULTLOG_H__

#include <ctime>
#include <cstdint>

class FaultLog
{
public :
    FaultLog(){};
    ~FaultLog(){};
       
    uint8_t id;
    uint16_t entryNo;
    time_t logTime;
    uint8_t errorCode;
    uint8_t onset;
};

#endif

