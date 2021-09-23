#include <cstdio>
#include <cstring>
#include <ctime>

#include <uci.h>
#include <module/Utils.h>
#include <uci/UciFault.h>
#include <module/MyDbg.h>

using namespace Utils;

UciFault::UciFault()
{
    PATH = "./config";
    PACKAGE = "UciFault";
    SECTION = "flt";
    faultLog = new FaultLog[FAULT_LOG_ENTRIES];
    for (int i = 0; i < FAULT_LOG_ENTRIES; i++)
    {
        faultLog[i].logTime = -1;
    }
    lastLog = -1;
}

UciFault::~UciFault()
{
    delete [] faultLog;
}

void UciFault::LoadConfig()
{
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    struct uci_element *e;
    struct uci_option *option;
    uint8_t buf[FAULT_LOG_SIZE];

    uci_foreach_element(&uciSec->options, e)
    {
        if (memcmp(e->name, "flt_", 4) != 0)
        {
            continue;
        }
        struct uci_option *option = uci_to_option(e);
        int i = atoi(e->name + 4);
        if (i < 0 || i >= FAULT_LOG_ENTRIES || option->type != uci_option_type::UCI_TYPE_STRING)
        {
            continue;
        }
        int len = strlen(option->v.string);
        if (len != FAULT_LOG_SIZE ||
            Cnvt::ParseToU8(option->v.string, buf, FAULT_LOG_SIZE) != 0 ||
            Crc::Crc16_1021(buf, FAULT_LOG_SIZE - 2) != Cnvt::GetU16(buf + (FAULT_LOG_SIZE - 2)))
        {
            continue;
        }
        uint8_t *p = &buf[0];
        faultLog[i].logTime = Cnvt::GetU32(p);
        p += 4;
        faultLog[i].entryNo = *p++;
        faultLog[i].errorCode = *p++;
        faultLog[i].onset = *p++;
    }

    try
    {
        lastLog = GetInt(uciSec, _LastLog, 0, FAULT_LOG_ENTRIES - 1);
    }
    catch (...)
    {
        time_t t = 0;
        for (int i = 0; i < FAULT_LOG_ENTRIES; i++)
        {
            if (faultLog[i].logTime > t)
            {
                lastLog = i;
                t = faultLog[i].logTime;
            }
        }
    }

    Close();
}

void UciFault::Dump()
{
}

int UciFault::GetFaultLog003(uint8_t *dst)
{
    if (lastLog < 0)
    {
        return 0;
    }
    uint8_t *p = dst + 1;
    int cnt = 0;
    int log = lastLog;
    for (int i = 0; i < FAULT_LOG_ENTRIES && cnt < 20; i++)
    {
        if (faultLog[log].logTime >= 0)
        {
            *p++ = faultLog[log].id;
            *p++ = faultLog[log].entryNo & 0xFF;
            p = Cnvt::PutLocalTm(faultLog[log].logTime, p);
            *p++ = faultLog[log].errorCode;
            *p++ = faultLog[log].onset;
            cnt++;
            if (--log < 0)
            {
                log = FAULT_LOG_ENTRIES - 1;
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
        return 0;
    }
    uint8_t *p = dst + 2;
    int cnt = 0;
    int log = lastLog;
    for (int i = 0; i < FAULT_LOG_ENTRIES; i++)
    {
        if (faultLog[log].logTime >= 0)
        {
            *p++ = faultLog[log].id;
            p = Cnvt::PutU16(faultLog[log].entryNo, p);
            p = Cnvt::PutLocalTm(faultLog[log].logTime, p);
            *p++ = faultLog[log].errorCode;
            *p++ = faultLog[log].onset;
            cnt++;
            if (--log < 0)
            {
                log = FAULT_LOG_ENTRIES - 1;
            }
        }
    }
    dst[0] = cnt / 0x100;
    dst[1] = cnt & 0xFF;
    return p - dst;
}

void UciFault::Push(uint8_t id, uint8_t errorCode, uint8_t onset)
{
    uint16_t entryNo = 0;
    if (lastLog != -1)
    {
        entryNo = faultLog[lastLog].entryNo + 1;
    }
    lastLog++;
    if (lastLog >= FAULT_LOG_ENTRIES)
    {
        lastLog = 0;
    }

    faultLog[lastLog].id = id;
    faultLog[lastLog].entryNo = entryNo;
    time_t t;
    faultLog[lastLog].logTime = t;
    faultLog[lastLog].errorCode = errorCode;
    faultLog[lastLog].onset = onset;

    uint8_t buf[FAULT_LOG_SIZE];
    uint8_t *p = &buf[0];
    *p++ = id;
    *p++ = entryNo;
    p = Cnvt::PutU32(t, p);
    *p++ = errorCode;
    *p++ = onset;

    uint16_t crc = Crc::Crc16_1021(buf, FAULT_LOG_SIZE - 2);
    Cnvt::PutU16(crc, buf + (FAULT_LOG_SIZE - 2));

    char v[FAULT_LOG_SIZE * 2 + 1];
    Cnvt::ParseToStr(buf, v, FAULT_LOG_SIZE);
    char option[16];
    sprintf(option, "flt_%d", lastLog);
    OpenSaveClose(SECTION, option, v);

    sprintf(v, "%d", lastLog);
    OpenSaveClose(SECTION, _LastLog, v);
}
