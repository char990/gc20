#include <cstdio>
#include <cstring>
#include "module/Tz_AU.h"

const Tz_AU::timezone_t Tz_AU::tz_au[NUMBER_OF_TZ] = {
    {"Perth", "AWST-8", 8 * 60 * 60, 0, {6 * 60 + 49, 7 * 60 + 16, 17 * 60 + 20, 17 * 60 + 47, 4 * 60 + 38, 5 * 60 + 7, 19 * 60 + 21, 19 * 60 + 50}},                                      // 0 Perth
    {"Brisbane", "AEST-10", 10 * 60 * 60, 0, {6 * 60 + 12, 6 * 60 + 37, 17 * 60 + 2, 17 * 60 + 27, 4 * 60 + 23, 4 * 60 + 49, 18 * 60 + 42, 19 * 60 + 9}},                                  // 1 Brisbane
    {"Darwin", "ACST-9:30", (9 * 60 + 30) * 60, 0, {6 * 60 + 43, 7 * 60 + 6, 18 * 60 + 30, 18 * 60 + 53, 5 * 60 + 55, 6 * 60 + 19, 19 * 60 + 10, 19 * 60 + 33}},                           // 2 Darwin
    {"Adelaide", "ACST-9:30ACDT,M10.1.0/2,M4.1.0/3", (9 * 60 + 30) * 60, 1, {6 * 60 + 55, 7 * 60 + 23, 17 * 60 + 12, 17 * 60 + 40, 4 * 60 + 29, 4 * 60 + 58, 19 * 60 + 29, 19 * 60 + 59}}, // 3 Adelaide
    {"Sydney", "AEST-10AEDT,M10.1.0/2,M4.1.0/3", 10 * 60 * 60, 1, {6 * 60 + 32, 7 * 60 + 0, 16 * 60 + 54, 17 * 60 + 21, 4 * 60 + 11, 4 * 60 + 40, 19 * 60 + 6, 19 * 60 + 35}},             // 4 Sydney
    {"Canberra", "AEST-10AEDT,M10.1.0/2,M4.1.0/3", 10 * 60 * 60, 1, {6 * 60 + 44, 7 * 60 + 12, 16 * 60 + 58, 17 * 60 + 27, 4 * 60 + 15, 4 * 60 + 45, 19 * 60 + 18, 19 * 60 + 48}},         // 5 Canberra
    {"Melbourne", "AEST-10AEDT,M10.1.0/2,M4.1.0/3", 10 * 60 * 60, 1, {7 * 60 + 6, 7 * 60 + 35, 17 * 60 + 8, 17 * 60 + 38, 4 * 60 + 23, 4 * 60 + 55, 19 * 60 + 42, 20 * 60 + 13}},          // 6 Melbourne
    {"Hobart", "AEST-10AEDT,M10.1.0/2,M4.1.0/3", 10 * 60 * 60, 1, {7 * 60 + 9, 7 * 60 + 41, 16 * 60 + 42, 17 * 60 + 15, 3 * 60 + 52, 4 * 60 + 27, 19 * 60 + 48, 20 * 60 + 23}},            // 7 Hobart
    {"Eucla", "ACWST-8:45", (8 * 60 + 45) * 60, 0, {6 * 60 + 41, 7 * 60 + 8, 17 * 60 + 13, 17 * 60 + 40, 4 * 60 + 32, 5 * 60 + 0, 19 * 60 + 14, 19 * 60 + 42}}                             // 8 Eucla
};

#define TW_START 0
#define TW_END 1
#define TW_DAWN 0
#define TW_DUSK 1
#define TW_JUN21 0
#define TW_DEC21 1

Tz_AU::Tz_AU(const char *city, const char *twilight_string)
{
    if (Init_Tz(city, twilight_string) < 0)
    {
        throw "Tz_AU constructor failed";
    }
}

Tz_AU::Tz_AU(unsigned char i, const char *twilight_string)
{
    if (Init_Tz(i, twilight_string) < 0)
    {
        throw "Tz_AU constructor failed";
    }
}

Tz_AU::Tz_AU(const char *city)
{
    if (Init_Tz(city) < 0)
    {
        throw "Tz_AU constructor failed";
    }
}

Tz_AU::Tz_AU(unsigned char i)
{
    if (Init_Tz(i) < 0)
    {
        throw "Tz_AU constructor failed";
    }
}

int Tz_AU::Init_Tz(const char *city, const char *twilight_string)
{
    if (InitTzIndex(city) < 0)
    {
        return -1;
    }
    if (InitTwilight(twilight_string) < 0)
    {
        return -2;
    }
    GetTwilightStatus(0);
    return 0;
}

int Tz_AU::Init_Tz(unsigned char i, const char *twilight_string)
{
    if (Init_Tz(i) < 0)
    {
        return -1;
    }
    if (InitTwilight(twilight_string) < 0)
    {
        return -2;
    }
    GetTwilightStatus(0);
    return 0;
}

int Tz_AU::Init_Tz(const char *city)
{
    if (InitTzIndex(city) < 0)
    {
        return -1;
    }
    InitTwilight(NULL);
    GetTwilightStatus(0);
    return 0;
}

int Tz_AU::Init_Tz(unsigned char i)
{
    if (i > 8)
    {
        return -1;
    }
    tz_index = i;
    InitTwilight(NULL);
    GetTwilightStatus(0);
    return 0;
}

int Tz_AU::InitTzIndex(const char *city)
{
    if (city != nullptr)
    {
        for (int i = 0; i < NUMBER_OF_TZ; i++)
        {
            if (strcmp(tz_au[i].city, city) == 0)
            {
                tz_index = i;
                return 0;
            }
        }
    }
    return -1;
}

int Tz_AU::InitTwilight(const char *twilight_string)
{
    if (_DecodeTwilightString(twilight_string) < 0)
    { // default
        memcpy(twilight_time, tz_au[tz_index].twilight, sizeof(twilight_time));
        return -1;
    }
    return 0;
}

const char *Tz_AU::GetTz()
{
    return tz_au[tz_index].TZ;
}

int Tz_AU::_DecodeTwilightString(const char *tt)
{
    if (tt == NULL)
        return -1;
    const char *p = tt;
    int h[8], m[8];
    for (int i = 0; i < 8; i++)
    {
        if (sscanf(p, "%d:%d", &h[i], &m[i]) != 2)
        {
            return -1;
        }
        if (h[i] < 0 || h[i] > 23 || m[i] < 0 || m[i] > 59)
        {
            return -1;
        }
        if (i != 0 && i != 4)
        {
            int t1 = h[i - 1] * 60 + m[i - 1];
            int t2 = h[i] * 60 + m[i];
            if (t1 >= t2)
            {
                return -1;
            }
        }
        if (i != 7)
        {
            p = strchr(p, ',');
            if (p == NULL)
            {
                return -1;
            }
            p++;
        }
    }
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                int n = (i * 2 + j) * 2 + k;
                twilight_time[i][j][k] = h[n] * 60 + m[n];
            }
        }
    }
    return 0;
}

enum Tz_AU::TwilightStatus Tz_AU::GetTwilightStatus(time_t t)
{
    static time_t today3am = 0; // avoiding DST offset, start from 3:00:01
    static time_t dd_se[2][2];
    if (t == 0)
    { // clear static
        today3am = 0;
        return TW_ST_NIGHT;
    }
    if (t < today3am || t >= (today3am + 24 * 3600))
    { // calculate new twilight time
        struct tm tm_tmp, today;
        if (localtime_r(&t, &tm_tmp) != &tm_tmp)
        {
            return TW_ST_NIGHT;
        }
        tm_tmp.tm_isdst = -1;
        tm_tmp.tm_hour = 3;
        tm_tmp.tm_min = 0;
        tm_tmp.tm_sec = 1;
        today3am = mktime(&tm_tmp);                                            // today 3:00:01
        localtime_r(&today3am, &today);                                        // today.tm_isdst is ready for dawn & dusk
        time_t today_st_0 = today3am - 1 - ((today.tm_isdst == 1) ? 2 : 3) * 3600; // standard time 0:00:00

        if (today.tm_mday == 21 && (today.tm_mon == 5 || today.tm_mon == 11))
        {
            int mon = (today.tm_mon == 5) ? TW_JUN21 : TW_DEC21;
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    dd_se[i][j] = today_st_0 + twilight_time[mon][i][j] * 60;
                }
            }
        }
        else
        {
            struct tm Jun21, Dec21;
            time_t t21;
            tm_tmp.tm_mday = 21;

            tm_tmp.tm_mon = 5; //Jun-21
            t21 = mktime(&tm_tmp);
            localtime_r(&t21, &Jun21);

            tm_tmp.tm_mon = 11; //Dec-21
            t21 = mktime(&tm_tmp);
            localtime_r(&t21, &Dec21);

            int half_year = 183; // days of half year
            if( (today.tm_yday<Jun21.tm_yday && (tm_tmp.tm_year&0x03)!=0) || // before Jun 21 of this year
                (today.tm_yday>Dec21.tm_yday && (tm_tmp.tm_year&0x03)!=3))   // after Dec 21 of this year
            {// not leap year
                half_year=182;
            }

            int days;
            int _start, _end;
            if (today.tm_yday < Jun21.tm_yday)
            {
                _start = TW_DEC21;
                _end = TW_JUN21;
                days = half_year - (Jun21.tm_yday - today.tm_yday);
            }
            else if (today.tm_yday > Dec21.tm_yday)
            {
                _start = TW_DEC21;
                _end = TW_JUN21;
                days = today.tm_yday - Dec21.tm_yday;
            }
            else
            {
                _start = TW_JUN21;
                _end = TW_DEC21;
                days = today.tm_yday - Jun21.tm_yday;
            }
            for (int i = 0; i < 2; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    int ttm; // twilight time linearly interpolated offset : minute
                    ttm = twilight_time[_end][i][j] - twilight_time[_start][i][j];
                    int everydaysec=(ttm * 60 * days / half_year);
                    dd_se[i][j] = today_st_0 + twilight_time[_start][i][j] * 60 + everydaysec;
                }
            }
        }
        /*
        printf("New twilight time:");
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                auto sec = dd_se[i][j]-today_st_0;
                printf(" %d:%02d:%02d", sec/3600, sec/60%60, sec%60);
            }
        }
        printf("\n");*/
    }
    if (t < dd_se[TW_DAWN][TW_START])
        return TW_ST_NIGHT;
    if (t < dd_se[TW_DAWN][TW_END])
        return TW_ST_DAWN;
    if (t < dd_se[TW_DUSK][TW_START])
        return TW_ST_DAY;
    if (t < dd_se[TW_DUSK][TW_END])
        return TW_ST_DUSK;
    return TW_ST_NIGHT;
}
