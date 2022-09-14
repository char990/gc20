#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <uci.h>
#include <module/Utils.h>
#include <uci/UciStrLog.h>
#include <module/MyDbg.h>
#include <cstdarg>

using namespace Utils;
extern time_t GetTime(time_t *);

void UciStrLog::LoadConfig()
{
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    struct uci_element *e;
    struct uci_option *option;
    char *p;
    int id, entryNo;
    time_t t;
    uci_foreach_element(&uciSec->options, e)
    {
        if (memcmp(e->name, _Log, 4) != 0)
        {
            continue;
        }
        option = uci_to_option(e);
        int i = atoi(e->name + 4);
        if (i < 0 || i >= pStrLog.size() || option->type != uci_option_type::UCI_TYPE_STRING)
        {
            continue;
        }
        if ((p = strchr(option->v.string, ']')) == nullptr)
        {
            continue;
        }
        if (sscanf(p, _Fmt_1, &t, &id, &entryNo) != 3)
        {
            continue;
        }
        if ((p = strstr(p, _Fmt_2)) == nullptr)
        {
            continue;
        }
        p += 4;
        auto &log = pStrLog[i];
        log.id = id;
        log.entryNo = entryNo;
        log.logTime = t;
        p = CharCpy(log.str, p, STRLOG_SIZE - 1);
        lastLog = i;
    }
    Close();
}

int UciStrLog::GetLog(uint8_t *dst)
{
    if (lastLog < 0)
    {
        dst[0] = 0;
        dst[1] = 0;
        return 2;
    }
    uint8_t *p = dst + 2;
    int logi = lastLog;
    int cnt = 0;
    for (int i = 0; i < pStrLog.size(); i++)
    {
        auto &log = pStrLog[logi];
        if (log.logTime >= 0)
        {
            *p++ = log.id;
            p = Cnvt::PutU16(log.entryNo, p);
            p = Time::PutLocalTime(log.logTime, p);
            p = CharCpy(p, log.str, STRLOG_SIZE - 1);
            p++;
            *p = '\0';
            cnt++;
        }
        if (--logi < 0)
        {
            logi = pStrLog.size() - 1;
        }
    }
    Cnvt::PutU16(cnt, dst);
    return p - dst;
}

int UciStrLog::GetLog(nlohmann::json & reply)
{
    int logi = lastLog;
    std::vector<nlohmann::json> items;
    if (lastLog >= 0)
    {
        for (int i = 0; i < pStrLog.size(); i++)
        {
            auto &log = pStrLog[logi];
            if (log.logTime >= 0)
            {
                nlohmann::json entry;
                entry.emplace("id", log.id);
                entry.emplace("entry_no", log.entryNo);
                entry.emplace("time", Time::ParseTimeToLocalStr(log.logTime));
                entry.emplace("content", log.str);
                items.push_back(entry);
            }
            if (--logi < 0)
            {
                logi = pStrLog.size() - 1;
            }
        }
    }
    reply.emplace("number_of_entries", items.size());
    reply.emplace("logs", items);
    return 0;
}

void UciStrLog::Push(uint8_t id, const char *fmt, ...)
{
    time_t t = GetTime(NULL);
    uint16_t entryNo = 0;
    if (lastLog != -1)
    {
        entryNo = pStrLog.at(lastLog).entryNo + 1;
    }
    lastLog++;
    if (lastLog >= pStrLog.size())
    {
        lastLog = 0;
    }

    auto &log = pStrLog[lastLog];
    log.id = id;
    log.entryNo = entryNo;
    log.logTime = t;
    va_list args;
    va_start(args, fmt);
    vsnprintf(log.str, STRLOG_SIZE - 1, fmt, args);
    va_end(args);

    char option[16];
    sprintf(option, "%s%d", _Log, lastLog);
    char v[128];
    v[0] = '[';
    char *p = Time::ParseTimeToLocalStr(t, v + 1);
    snprintf(p, 127 - (p - v), _Fmt_3, t, id, entryNo, log.str);

    OpenSaveClose(SECTION, option, v);
}

void UciStrLog::Reset()
{
    lastLog = -1;
    for (auto &s : pStrLog)
    {
        s.logTime = -1;
    }
    UciCfg::ClrSECTION();
}
