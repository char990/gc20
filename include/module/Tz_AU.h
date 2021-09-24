#pragma once

#include <ctime>

// char * twilight_string:
// All Twilight Time is local Civil Twilight time without applying DST offset
// {Jun21},                     {Dec21}
// {{Dawn},             {Dusk}}
// {{{Start},   {End}}
// {{{{hh:mm}}}}
// "6:30,6:50, ...... ,18:24,19:49" = total 8 of hh:mm
#define NUMBER_OF_TZ 9
class Tz_AU
{
public:
    Tz_AU(char *city, char *twilight_string);
    Tz_AU(char *city);
    Tz_AU(unsigned char i);
    void Init_Tz(char *city, char *twilight_string);
    void Init_Tz(char *city);
    void Init_Tz(unsigned char i);
    const char *GetTz(void);
    int GetTzIndex(void) { return tz_index; };
    enum TwilightStatus
    {
        TW_ST_NIGHT,
        TW_ST_DAWN,
        TW_ST_DAY,
        TW_ST_DUSK
    };
    enum TwilightStatus GetTwilightStatus(time_t t);

    typedef struct timezone_t
    {
        const char *city;
        const char *TZ;
        int gmt_offset;
        int dst;
        int twilight[2][2][2]; // minute from standard time 0:00:00
    } timezone_t;

    // All Twilight Time is local Civil Twilight time without applying DST offset
    // 0-7: Data from AS 4852.1:2009 - TABLE 3.2
    // 8: Data from year 2020
    static const timezone_t tz_au[NUMBER_OF_TZ];

private:
    void InitTwilight(char *twilight_string);
    void InitTzIndex(char *city);
    int tz_index;
    int twilight_time[2][2][2];
    int _DecodeTwilightString(char *tt);
};

