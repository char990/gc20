#include <cstdio>
#include <cstring>
#include <ctime>


#include <uci.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

using namespace Utils;

UciFault::UciFault()
{
    lastLog = -1;
}

UciFault::~UciFault()
{
}

void UciFault::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciFault";
    SECTION = "flt";
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    struct uci_element *e;
    struct uci_option *option;
    time_t t;
    int id, entryNo, errorCode, onset, crc;
    char *p;

    uci_foreach_element(&uciSec->options, e)
    {
        if (memcmp(e->name, _Log, 4) != 0)
        {
            continue;
        }
        struct uci_option *option = uci_to_option(e);
        int i = atoi(e->name + 4);
        if (i < 0 || i >= faultLog.size() || option->type != uci_option_type::UCI_TYPE_STRING)
        {
            continue;
        }
        if ((p = strchr(option->v.string, ']')) == nullptr)
        {
            continue;
        }
        if (sscanf(p, _Fmt, &t, &id, &entryNo, &errorCode, &onset, &crc) != 6)
        {
            continue;
        }
        auto & log = faultLog.at(i);
        log.id = id;
        log.logTime = t;
        log.entryNo = entryNo;
        log.errorCode = errorCode;
        log.onset = onset;
        if (log.MakeCrc() != crc)
        {
            log.logTime = -1; // logTIme=-1 means log is invalid
            continue;
        }
        lastLog = i;
    }
    Close();
}

void UciFault::Dump()
{
}

int UciFault::GetFaultLog20(uint8_t *dst)
{
    if (lastLog < 0)
    {
        dst[0]=0;
        return 1;
    }
    uint8_t *p = dst + 1;
    int cnt = 0;
    int logi = lastLog;
    for (int i = 0; i < faultLog.size() && cnt<20; i++)
    {
        auto &log = faultLog.at(logi);
        if (log.logTime >= 0)
        {
            *p++ = log.id;
            *p ++ = log.entryNo;
            p = Cnvt::PutLocalTm(log.logTime, p);
            *p++ = log.errorCode;
            *p++ = log.onset;
            cnt++;
        }
        if (--logi < 0)
        {
            logi = faultLog.size() - 1;
        }
    }
    dst[0]=cnt;
    return p - dst;
}

int UciFault::GetLog(uint8_t *dst)
{
    if (lastLog < 0)
    {
        dst[0]=0;
        dst[1]=0;
        return 2;
    }
    uint8_t *p = dst + 2;
    int cnt = 0;
    int logi = lastLog;
    for (int i = 0; i < faultLog.size(); i++)
    {
        auto &log = faultLog.at(logi);
        if (log.logTime >= 0)
        {
            *p++ = log.id;
            p = Cnvt::PutU16(log.entryNo, p);
            p = Cnvt::PutLocalTm(log.logTime, p);
            *p++ = log.errorCode;
            *p++ = log.onset;
            cnt++;
        }
        if (--logi < 0)
        {
            logi = faultLog.size() - 1;
        }
    }
    Cnvt::PutU16(cnt, dst);
    return p - dst;
}

void UciFault::Push(uint8_t id, DEV::ERROR errorCode, uint8_t onset, time_t t)
{
    if(t<0)
    {
        return;
    }
    uint16_t entryNo = 0;
    if (lastLog != -1)
    {
        entryNo = faultLog.at(lastLog).entryNo + 1;
    }
    if (++lastLog >= faultLog.size())
    {
        lastLog = 0;
    }
    auto &log = faultLog.at(lastLog);
    log.id = id;
    log.entryNo = entryNo;
    log.logTime = t;
    log.errorCode = static_cast<uint8_t>(errorCode);
    log.onset = onset;
    log.MakeCrc();

    char option[16];
    sprintf(option, "%s%d", _Log, lastLog);

    char v[128];
    v[0] = '[';
    char *p = Cnvt::ParseTmToLocalStr(t, v + 1);
    snprintf(p, 127-(p-v),  _Fmt, t, id, entryNo, log.errorCode, onset, log.crc);

    OpenSaveClose(SECTION, option, v);
    if(errorCode!=DEV::ERROR::ControllerResetViaWatchdog)
    {
        PrintDbg("Fault:%s:%s\n", DEV::ToStr(errorCode), onset?"onset":"clear");
    }
}

void UciFault::Push(uint8_t id, DEV::ERROR errorCode, uint8_t onset)
{
    Push(id, errorCode, onset, time(nullptr));
}

void UciFault::Reset()
{
    lastLog=-1;
    for(int i=0;i<faultLog.size();i++)
    {
        faultLog.at(i).logTime=-1;
    }
    UciCfg::ClrSECTION();
}
