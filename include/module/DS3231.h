#pragma once

/*****************************************************************************
DS3231 deal with the time from 1901-1-1 0:00:00 to 2099-12-31 23:59:59

1900 is not leap year, but DS3231 could present 1900-2-29  

There is no DST adjustment in DS3231

Consider the TIME in DS3231 as UTC
Translate it to localtime before using 

******************************************************************************/

#include <time.h>

class DS3231
{
public:
     DS3231(int bus);

     int GetLocalTime(struct tm *localtm);    // read DS3231 RTC to tm_t *
     int SetLocalTime(struct tm *localtm);   // write tm_t * to DS3231 regs[0-6]

     int GetUtcTime(struct tm *utctm);        // read DS3231 RTC to UTC tm_t *

     int SetRtcRegs(char *rtc);              // copy char * to DS3231 regs[0-6]
     
     int GetTimet(void);                        // read DS3231 RTC to time_t
     int SetTimet(time_t t);                 // write time_t to DS3231 regs[0-6]

     int GetTemp(int *t);       // t: -127 -> 127 'C

     int GetControl(char *v);       // Get controll register 0x0E
     int SetControl(char v);        // Set controll register 0x0E

     int GetStatus(char *v);       // Get Status register 0x0F
     int SetStatus(char v);        // Set Status register 0x0F

     int hex2bcd(int hex);
     int bcd2hex(int bcd);
     int WriteRegs(int addr, int *buf, int len);
     int ReadRegs(int addr, int *buf, int len);

     int WriteTimeAlarm(time_t t);
     int ReadTimeAlarm(time_t *t);

     int Print();
private:
     int _bus;
     void PrintTm(struct tm *tm);
};

extern DS3231 * pDS3231;
