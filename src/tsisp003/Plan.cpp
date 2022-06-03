#include <stdexcept>
#include <cstring>
#include <array>

#include <tsisp003/Plan.h>

#include <module/Utils.h>
#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <tsisp003/Frame.h>
#include <tsisp003/Message.h>

using namespace Utils;

APP::ERROR Plan::Init(uint8_t *xpln, int xlen)
{
    micode = 0;
    plnId = xpln[1];
    plnRev = xpln[2];
    weekdays = xpln[3];
    crc = Cnvt::GetU16(xpln + xlen - PLN_TAIL);
    if (xpln[0] != static_cast<uint8_t>(MI::CODE::SignSetPlan))
    {
        return APP::ERROR::UnknownMi;
    }
    if (xlen < (PLN_LEN_MIN + PLN_TAIL) || xlen > (PLN_LEN_MAX + PLN_TAIL)) // with crc & enable flag
    {
        Ldebug("Plan[%d] Error:len=%d", plnId, xlen);
        return APP::ERROR::LengthError;
    }
    if (plnId == 0)
    {
        Ldebug("Plan Error:PlanID=0");
        return APP::ERROR::SyntaxError;
    }
    if ((weekdays & 0x80) != 0 || (weekdays & 0x7F) == 0)
    {
        Ldebug("Plan[%d] Error:weekdays=0x%02X", plnId, weekdays);
        return APP::ERROR::SyntaxError;
    }
    uint8_t *p = xpln + 4;
    entries = 0;
    for (int i = 0; i < 6; i++)
    {
        plnEntries[i].fmType = *p++;
        if (plnEntries[i].fmType == PLN_ENTRY_NA)
        {
            if (i == 0)
            {
                Ldebug("Plan[%d] Error:type of first entry=0", plnId);
                return APP::ERROR::LengthError;
            }
            break;
        }
        else if (plnEntries[i].fmType > PLN_ENTRY_MSG)
        {
            return APP::ERROR::SyntaxError;
        }
        plnEntries[i].fmId = *p++;
        plnEntries[i].start.hour = *p++;
        plnEntries[i].start.min = *p++;
        plnEntries[i].stop.hour = *p++;
        plnEntries[i].stop.min = *p++;
        if (plnEntries[i].start.hour > 23 ||
            plnEntries[i].start.min > 59 ||
            plnEntries[i].stop.hour > 23 ||
            plnEntries[i].stop.min > 59)
        {
            return APP::ERROR::SyntaxError;
        }
        entries++;
    }
    if (p != (xpln + xlen - PLN_TAIL))
    {
        Ldebug("Plan[%d] Error:invalid entries", plnId);
        return APP::ERROR::LengthError;
    }
    // check time overlap in plan
    int start[6];
    int stop[6];
    for (int i = 0; i < entries; i++)
    {
        start[i] = plnEntries[i].start.hour * 60 + plnEntries[i].start.min;
        stop[i] = plnEntries[i].stop.hour * 60 + plnEntries[i].stop.min;
        if (start[i] >= stop[i])
        {
            stop[i] += 24 * 60;
        }
    }
    for (int i = 0; i < entries - 1; i++)
    {
        for (int j = i + 1; j < entries; j++)
        {
            if ((start[i] >= start[j] && start[i] < stop[j]) ||
                (stop[i] > start[j] && stop[i] <= stop[j]))
            {
                return APP::ERROR::OverlaysNotSupported;
            }
        }
    }
    if (0 != CheckEntries())
    {
        return APP::ERROR::FrmMsgPlnUndefined;
    }
    micode = static_cast<uint8_t>(MI::CODE::SignSetPlan);
    return APP::ERROR::AppNoError;
}

int Plan::ToArray(uint8_t *pbuf)
{
    uint8_t *p = pbuf;
    *p++ = micode;
    *p++ = plnId;
    *p++ = plnRev;
    *p++ = weekdays;
    for (int i = 0; i < 6; i++)
    {
        *p++ = plnEntries[i].fmType;
        if (plnEntries[i].fmType == PLN_ENTRY_NA)
        {
            break;
        }
        *p++ = plnEntries[i].fmId;
        *p++ = plnEntries[i].start.hour;
        *p++ = plnEntries[i].start.min;
        *p++ = plnEntries[i].stop.hour;
        *p++ = plnEntries[i].stop.min;
    }
    p = Cnvt::PutU16(crc, p);
    return p - pbuf;
}

std::string Plan::ToString()
{
    if (micode == 0)
    {
        return "Plan undefined";
    }
    #define PLAN_TO_STRING_SIZE 4096
    char *buf = new char[PLAN_TO_STRING_SIZE+16];
    int len = 0;
    len = snprintf(buf, PLAN_TO_STRING_SIZE, "Pln_%03d: MI=0x%02X, Id=%d, Rev=%d, Weekday=",
                   plnId, micode, plnId, plnRev);
    const char *WEEK[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    for (int i = 0, k = 1; i < 7; i++)
    {
        if (k & weekdays)
        {
            len += snprintf(buf + len, PLAN_TO_STRING_SIZE - len, "%s,", WEEK[i]);
        }
        k <<= 1;
    }
    len += snprintf(buf + len, PLAN_TO_STRING_SIZE - len, " Entries(%d)=", entries);
    for (int i = 0; i < 6; i++)
    {
        if (plnEntries[i].fmType == PLN_ENTRY_FRM)
        {
            len += snprintf(buf + len, PLAN_TO_STRING_SIZE - len, "(Frm");
        }
        else if (plnEntries[i].fmType == PLN_ENTRY_MSG)
        {
            len += snprintf(buf + len, PLAN_TO_STRING_SIZE - len, "(Msg");
        }
        else
        {
            break;
        }
        len += snprintf(buf + len, PLAN_TO_STRING_SIZE - len, "[%d]%d:%02d-%d:%02d)", plnEntries[i].fmId,
                        plnEntries[i].start.hour, plnEntries[i].start.min,
                        plnEntries[i].stop.hour, plnEntries[i].stop.min);
    }
    snprintf(buf + len, PLAN_TO_STRING_SIZE - len, ", Crc=0x%04X", crc);
    std::string s(buf);
    delete [] buf;
    return s;
}

int Plan::CheckEntries()
{
    for (int i = 0; i < entries; i++)
    {
        if (plnEntries[i].fmType == PLN_ENTRY_FRM && !DbHelper::Instance().GetUciFrm().IsFrmDefined(plnEntries[i].fmId))
        {
            Ldebug("Plan[%d] Error:Frame[%d] undefined", plnId, plnEntries[i].fmId);
            return -1;
        }
        else if (plnEntries[i].fmType == PLN_ENTRY_MSG && !DbHelper::Instance().GetUciMsg().IsMsgDefined(plnEntries[i].fmId))
        {
            Ldebug("Plan[%d] Error:Msg[%d] undefined", plnId, plnEntries[i].fmId);
            return -1;
        }
    }
    return 0;
}
