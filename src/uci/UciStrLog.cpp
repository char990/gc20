#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <uci.h>
#include <module/Utils.h>
#include <uci/UciStrLog.h>
#include <module/MyDbg.h>

using namespace Utils;

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
        if (i < 0 || i >= maxEntries || option->type != uci_option_type::UCI_TYPE_STRING)
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
        if (sscanf(p, _Fmt_1, &id, &entryNo) != 2)
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
        p=CharCpy(log.str, p, STR_SIZE - 1);
        lastLog = i;
    }
    Close();
}

int UciStrLog::GetLog(uint8_t *dst)
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
    for (int i = 0; i < maxEntries; i++)
    {
        auto &log = pStrLog[logi];
        if (log.logTime >= 0)
        {
            *p++ = log.id;
            p = Cnvt::PutU16(log.entryNo, p);
            p = Cnvt::PutLocalTm(log.logTime, p);
            p = CharCpy(p, log.str, STR_SIZE - 1);
            cnt++;
            if (--logi < 0)
            {
                logi = maxEntries - 1;
            }
        }
    }
    Cnvt::PutU16(cnt, dst);
    return p - dst;
}

void UciStrLog::Push(uint8_t id, const char *pbuf)
{
    uint16_t entryNo = 0;
    if (lastLog != -1)
    {
        entryNo = (pStrLog + lastLog)->entryNo + 1;
    }
    lastLog++;
    if (lastLog >= maxEntries)
    {
        lastLog = 0;
    }

    char *p;
    auto &log = pStrLog[lastLog];
    time_t t = time(NULL);
    log.id = id;
    log.entryNo = entryNo;
    log.logTime = t;
    p = CharCpy(log.str, pbuf, STR_SIZE - 1);

    char option[16];
    sprintf(option, "%s%d", _Log, lastLog);

    char v[128];
    v[0] = '[';
    p = Cnvt::ParseTmToLocalStr(t, v + 1);
    sprintf(p, _Fmt_3, id, entryNo, log.str);

    OpenSaveClose(SECTION, option, v);
}

void UciStrLog::Reset()
{
    lastLog=-1;
    for(int i=0;i<maxEntries;i++)
    {
        pStrLog[i].logTime=-1;
    }
    char dst[256];
    sprintf(dst, "%s/%s", PATH, PACKAGE);
    int dstfd = open(dst, O_WRONLY | O_TRUNC, 0660);
    if (dstfd < 0)
    {
        MyThrow("Can't open %s to write", dst);
    }
    int len = sprintf(dst, "config %s '%s'\n", PACKAGE, SECTION);
    write(dstfd, &dst[0], len);
    close(dstfd);
}
