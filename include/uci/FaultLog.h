#pragma once


#include <ctime>
#include <cstdint>

#define FAULT_LOG_SIZE 10
class FaultLog
{
public :
    FaultLog(){};
    ~FaultLog(){};
       
    time_t logTime{-1};      // logTIme=-1 means log is invalid
    uint8_t id{0};
    uint16_t entryNo{0};
    uint8_t errorCode{0};
    uint8_t onset{0};
    uint16_t crc;
    uint16_t MakeCrc()
    {
        uint8_t buf[8];
        uint8_t *p = &buf[0];
        *p++ = id;
        *p++ = entryNo;
        p = Utils::Cnvt::PutU32(logTime, p);
        *p++ = errorCode;
        *p++ = onset;
        crc = Utils::Crc::Crc16_1021(buf, 8);
        return crc;
    };
};
