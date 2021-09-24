#pragma once


#include <ctime>

#define STR_SIZE 64

class StrLog
{
public :
    StrLog(){};
    ~StrLog(){};

    uint8_t id;
    uint16_t entryNo;
    time_t logTime;
    char str[STR_SIZE];
};



