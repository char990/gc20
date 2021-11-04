#pragma once

#include <uci/UciLog.h>
#include <uci/StrLog.h>
#include <vector>

/*
    # log_xxx : xxx is log ID
    // string, max length is 63
    option log_0  "[d/m/yyyy h:mm:ss]ID=%d, EntryNo=%d, Str=%s"
*/

class UciStrLog : public UciLog
{
public:
    UciStrLog(){};
    UciStrLog(int max):pStrLog(max){};
    virtual ~UciStrLog(){};

    virtual void LoadConfig() override;

    virtual int GetLog(uint8_t *dst);

    /// \brief  Push a string log into Log and save to Ucixxx
    virtual void Push(uint8_t id, const char *fmt, ...);

    void Reset();

protected:
    std::vector<StrLog> pStrLog;
    const char * _Log = "log_";
    const char * _Fmt_1 =  "]t=%d, ID=%d, EntryNo=%d, ";
    const char * _Fmt_2 =  "Str=";
    const char * _Fmt_3 =  "]t=%d, ID=%d, EntryNo=%d, Str=%s";
};
