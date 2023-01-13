#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "3rdparty/i2clib/i2c-dev.h"
#include "3rdparty/i2clib/i2clib.h"
#include "module/DS3231.h"

DS3231 *pDS3231;

/****************************************
 The time in DS3231 registers is UTC time
 ****************************************/
DS3231::DS3231(int bus)
{
    _bus = bus;
}

int DS3231::bcd2hex(int bcd)
{
    return (bcd & 0x0F) + ((bcd >> 4) & 0x0F) * 10;
}

int DS3231::hex2bcd(int hex)
{
    int x1, x10;
    x1 = hex % 10;
    x10 = ((hex - x1) / 10) % 10;
    return x10 * 0x10 + x1;
}

int DS3231::ReadRegs(int addr, int len, unsigned char *buf)
{
    return rd_i2c(_bus, 0x68, addr, len, buf);
}

int DS3231::WriteRegs(int addr, int len, const unsigned char *buf)
{
    return wr_i2c(_bus, 0x68, addr, len, buf);
}

int DS3231::GetUtcTime(struct tm *utctm)
{
    unsigned char reg[7];
    int result;
    result = ReadRegs(0x00, 7, reg);
    if (result < 7)
    {
        return -1;
    }
    utctm->tm_isdst = -1;
    utctm->tm_sec = bcd2hex(reg[0]);
    utctm->tm_min = bcd2hex(reg[1]);
    if (reg[2] & 0x40)
    { // 12hours
        utctm->tm_hour = bcd2hex(reg[2] & 0x1F);
        if ((reg[2] & 0x20) == 0)
        { // am
            if (utctm->tm_hour == 12)
            {
                utctm->tm_hour = 0; // 12am = 0
            }
        }
        else
        { // pm
            if (utctm->tm_hour < 12)
            {
                utctm->tm_hour += 12; // 1-11pm = 13-23
            }
        }
    }
    else
    {
        utctm->tm_hour = bcd2hex(reg[2]);
    }
    utctm->tm_wday = bcd2hex(reg[3]) - 1;
    utctm->tm_mday = bcd2hex(reg[4]);
    utctm->tm_mon = bcd2hex(reg[5] & 0x1F) - 1;
    utctm->tm_year = bcd2hex(reg[6]) + ((reg[5] & 0x80) ? 100 : 0);
    return (0);
}

int DS3231::GetLocalTime(struct tm *localtm)
{
    time_t t = GetTimet();
    if (t < 0)
    {
        return t;
    }
    localtime_r(&t, localtm);
    return (0);
}

int DS3231::SetLocalTime(struct tm *localtm)
{
    localtm->tm_isdst = -1;
    time_t t = mktime(localtm);
    if (t < 0)
        return t;
    return SetTimet(t);
}

int DS3231::GetTimet(void)
{
    struct tm utc;
    int result;
    result = GetUtcTime(&utc);
    if (result < 0)
    {
        return (result);
    }
    return timegm(&utc);
}

int DS3231::SetTimet(time_t t)
{
    struct tm utc;
    if (gmtime_r(&t, &utc) <= 0)
    {
        return -1;
    }
    changed = 1;
    unsigned char reg[7];
    reg[0] = hex2bcd(utc.tm_sec);
    reg[1] = hex2bcd(utc.tm_min);
    reg[2] = hex2bcd(utc.tm_hour);
    reg[3] = hex2bcd(utc.tm_wday + 1);
    reg[4] = hex2bcd(utc.tm_mday);
    if (utc.tm_year >= 100)
    {
        reg[5] = hex2bcd(utc.tm_mon + 1) | 0x80;
        reg[6] = hex2bcd(utc.tm_year - 100);
    }
    else
    {
        reg[5] = hex2bcd(utc.tm_mon + 1);
        reg[6] = hex2bcd(utc.tm_year);
    }
    return WriteRegs(0x00, 7, reg);
}

int DS3231::SetRtcRegs(char *rtc)
{
    changed = 1;
    unsigned char reg[7];
    for (int i = 0; i < 7; i++)
    {
        reg[i] = rtc[i];
    }
    return WriteRegs(0x00, 7, reg);
}

int DS3231::GetTemp(int *t)
{
    unsigned char tt;
    int result;
    result = ReadRegs(0x11, 1, &tt);
    if (result < 1)
    {
        return -1;
    }
    unsigned char t1 = tt;
    *t = *((int8_t *)&t1);
    return 0;
}

int DS3231::GetControl(char *v)
{
    unsigned char pr;
    int result;
    result = ReadRegs(0x0E, 1, &pr);
    if (result < 1)
    {
        return -1;
    }
    *v = pr;
    return 0;
}

int DS3231::SetControl(char v)
{
    return WriteRegs(0x0E, 1, (const unsigned char *)&v);
}

int DS3231::GetStatus(char *v)
{
    unsigned char pr;
    int result;
    result = ReadRegs(0x0F, 1, &pr);
    if (result < 1)
    {
        return -1;
    }
    *v = pr;
    return 0;
}

int DS3231::SetStatus(char v)
{
    return WriteRegs(0x0F, 1, (const unsigned char *)&v);
}

int DS3231::Print()
{
    struct tm tm;
    int temp;
    int result;
    result = GetUtcTime(&tm);
    if (result < 0)
    {
        printf("\nError: GetUtcTime=%d\n", result);
        return 1;
    }
    printf("UTCtime = ");
    PrintTm(&tm);
    time_t t = timegm(&tm);
    localtime_r(&t, &tm);
    printf("Localtime = ");
    PrintTm(&tm);
    result = GetTemp(&temp);
    if (result < 0)
    {
        printf("\nError: GetTemp=%d\n", result);
        return 1;
    }
    printf("GetTemp = \t%d'C\n", temp);
    ReadTimeAlarm(&t);
    localtime_r(&t, &tm);
    printf("TimeAlarm = ");
    PrintTm(&tm);
    return 0;
}

void DS3231::PrintTm(struct tm *tm)
{
    printf("\t%2d/%02d/%02d %2d:%02d:%02d%s\n",
           tm->tm_mday,
           tm->tm_mon + 1,
           tm->tm_year + 1900,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec,
           (tm->tm_isdst == 1) ? "(DST)" : "");
}

#define ALARM_FLAG 0x25
int DS3231::WriteTimeAlarm(time_t t)
{
    unsigned char reg[7];
    unsigned char rd = 1;
    WriteRegs(0x0D, 1, &rd);
    struct tm utctm;
    struct tm *r = gmtime_r(&t, &utctm);
    if (r == &utctm)
    {
        reg[0] = hex2bcd(utctm.tm_sec);
        reg[1] = hex2bcd(utctm.tm_min);
        reg[2] = hex2bcd(utctm.tm_hour);
        reg[3] = hex2bcd(utctm.tm_mday);
        reg[4] = hex2bcd(utctm.tm_year - 100);
        reg[5] = hex2bcd(utctm.tm_mon);
        reg[6] = ALARM_FLAG; // flag
                             /*printf("WriteAlarm : t=%ld %2d/%02d/%d %2d:%02d:%02d\nRegs:", t,
            utctm.tm_mday, utctm.tm_mon, utctm.tm_year-100, utctm.tm_hour, utctm.tm_min, utctm.tm_sec) ;
        for(int i=0;i<7;i++)
        {
            printf(" %02X",reg[i]);
        }
        printf("\n");
        */
        return WriteRegs(0x07, 7, reg);
    }
    return -1;
}

// return :
//	0: DS3231 OK
//		*t=-1: invalid time or flag is not 0x31
//		*t: utc timestamp
//	-1: DS3231 Error
int DS3231::ReadTimeAlarm(time_t *t)
{
    unsigned char reg[7];
    int r = ReadRegs(0x07, 7, reg);
    *t = -1;
    if (r == 7 && reg[6] == ALARM_FLAG)
    {
        struct tm utctm = {0}; // init to 0
        utctm.tm_isdst = -1;
        utctm.tm_sec = bcd2hex(reg[0] & 0x7F);
        utctm.tm_min = bcd2hex(reg[1] & 0x7F);
        utctm.tm_hour = bcd2hex(reg[2] & 0x3F);
        utctm.tm_mday = bcd2hex(reg[3] & 0x3F);       // 1-31
        utctm.tm_year = bcd2hex(reg[4] & 0x7F) + 100; // 0-59: => 2000-2059
        utctm.tm_mon = bcd2hex(reg[5] & 0x1F);        // 0-11
        *t = timegm(&utctm);                          // utc time -> time_t
                                                      /*printf("ReadTimeAlarm REgs:");
        for(int i=0;i<7;i++)
        {
            printf(" %d",reg[i]);
        }
        printf(" => %s", ctime(t));
        */
        return 0;
    }
    return -1;
}

bool DS3231::IsChanged()
{
    if (changed)
    {
        changed = 0;
        return true;
    }
    return false;
}