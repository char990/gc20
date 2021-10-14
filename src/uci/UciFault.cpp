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
    faultLog = new FaultLog[FAULT_LOG_ENTRIES];
    lastLog = -1;
}

UciFault::~UciFault()
{
    delete[] faultLog;
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
        if (i < 0 || i >= FAULT_LOG_ENTRIES || option->type != uci_option_type::UCI_TYPE_STRING)
        {
            continue;
        }
        if ((p = strchr(option->v.string, '[')) == nullptr)
        {
            continue;
        }
        if ((t = Cnvt::ParseLocalStrToTm(p + 1)) == -1)
        {
            continue;
        }
        if ((p = strchr(p, ']')) == nullptr)
        {
            continue;
        }
        if (sscanf(p, _Fmt, &id, &entryNo, &errorCode, &onset, &crc) != 5)
        {
            continue;
        }
        auto & log = faultLog[i];
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
    for (int i = 0; i < FAULT_LOG_ENTRIES && cnt < 20; i++)
    {
        auto &log = faultLog[logi];
        if (log.logTime >= 0)
        {
            *p++ = log.id;
            *p++ = log.entryNo & 0xFF;
            p = Cnvt::PutLocalTm(log.logTime, p);
            *p++ = log.errorCode;
            *p++ = log.onset;
            cnt++;
            if (--logi < 0)
            {
                logi = FAULT_LOG_ENTRIES - 1;
            }
        }
    }
    dst[0] = cnt;
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
    for (int i = 0; i < FAULT_LOG_ENTRIES; i++)
    {
        auto &log = faultLog[logi];
        if (log.logTime >= 0)
        {
            *p++ = log.id;
            p = Cnvt::PutU16(log.entryNo, p);
            p = Cnvt::PutLocalTm(log.logTime, p);
            *p++ = log.errorCode;
            *p++ = log.onset;
            cnt++;
            if (--logi < 0)
            {
                logi = FAULT_LOG_ENTRIES - 1;
            }
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
        entryNo = faultLog[lastLog].entryNo + 1;
    }
    if (++lastLog >= FAULT_LOG_ENTRIES)
    {
        lastLog = 0;
    }
    auto &log = faultLog[lastLog];
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
    sprintf(p, _Fmt, id, entryNo, log.errorCode, onset, log.crc);

    OpenSaveClose(SECTION, option, v);
}

void UciFault::Push(uint8_t id, DEV::ERROR errorCode, uint8_t onset)
{
    Push(id, errorCode, onset, time(nullptr));
}

void UciFault::Reset()
{
    lastLog=-1;
    for(int i=0;i<FAULT_LOG_ENTRIES;i++)
    {
        faultLog[i].logTime=-1;
    }
    UciCfg::ClrSECTION();
}
