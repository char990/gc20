#pragma once


#include <ctime>

#define STR_SIZE 64

class StrLog
{
public :
    StrLog(){};
    ~StrLog(){};

    uint8_t id{0};
    uint16_t entryNo{0};
    time_t logTime{-1};
    char str[STR_SIZE];
};



