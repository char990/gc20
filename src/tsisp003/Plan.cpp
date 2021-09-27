#include <stdexcept>
#include <cstring>

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
    if (xpln[0] != MI::CODE::SignSetPlan)
    {
        return APP::ERROR::UnknownMi;
    }
    if (xlen < (PLN_LEN_MIN + PLN_TAIL) || xlen > (PLN_LEN_MAX + PLN_TAIL)) // with crc & enable flag
    {
        return APP::ERROR::LengthError;
    }
    if (plnId == 0 || (weekdays & 0x80) != 0 || (weekdays & 0x7F) == 0)
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t *p = xpln + 4;
    entries=0;
    for (int i = 0; i < 6; i++)
    {
        plnEntries[i].type = *p++;
        if (plnEntries[i].type == 0)
        {
            if (i == 0)
            {
                return APP::ERROR::SyntaxError;
            }
            break;
        }
        plnEntries[i].fmId = *p++;
        plnEntries[i].start.hour = *p++;
        plnEntries[i].start.min = *p++;
        plnEntries[i].stop.hour = *p++;
        plnEntries[i].stop.min = *p++;
        entries++;
    }
    if( p != (xpln+xlen-PLN_TAIL) )
    {
        return APP::ERROR::SyntaxError;
    }
    if(0!=CheckEntries())
    {
        return APP::ERROR::FrmMsgPlnUndefined;
    }
    micode = MI::CODE::SignSetPlan;
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
        *p++ = plnEntries[i].type;
        if (plnEntries[i].type == 0)
        {
            break;
        }
        *p++ = plnEntries[i].fmId;
        *p++ = plnEntries[i].start.hour;
        *p++ = plnEntries[i].start.min;
        *p++ = plnEntries[i].stop.hour;
        *p++ = plnEntries[i].stop.min;
    }
    p=Cnvt::PutU16(crc,p);
    return p - pbuf;
}

std::string Plan::ToString()
{
    if (micode == 0)
    {
        return "Plan undefined";
    }
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "Pln_%03d: MI=0x%02X, Id=%d, Rev=%d, Weekday=",
                plnId, micode, plnId, plnRev);
    const char *WEEK[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    for (int i = 0, k = 1; i < 7; i++)
    {
        if (k & weekdays)
        {
            len += snprintf(buf + len, 1023 - len, "%s,", WEEK[i]);
        }
        k <<= 1;
    }
    len += snprintf(buf + len, 1023 - len, " Entries(%d)=", entries);
    for (int i = 0; i < 6; i++)
    {
        if (plnEntries[i].type == 1)
        {
            len += snprintf(buf + len, 1023 - len, "(Frm");
        }
        else if (plnEntries[i].type == 2)
        {
            len += snprintf(buf + len, 1023 - len, "(Msg");
        }
        else
        {
            break;
        }
        len += snprintf(buf + len, 1023 - len, "[%d]%d:%02d-%d:%02d)", plnEntries[i].fmId,
                        plnEntries[i].start.hour, plnEntries[i].start.min,
                        plnEntries[i].stop.hour, plnEntries[i].stop.min);
    }
    snprintf(buf + len, 1023 - len, ", Crc=0x%04X", crc);
    std::string s(buf);
    return s;
}

int Plan::CheckEntries()
{
    for (int i = 0; i < entries; i++)
    {
        if( (plnEntries[i].type == 1 && !DbHelper::Instance().GetUciFrm().IsFrmDefined(plnEntries[i].fmId)) ||
            (plnEntries[i].type == 2 && !DbHelper::Instance().GetUciMsg().IsMsgDefined(plnEntries[i].fmId)) )
        {
            return -1;
        }
    }
    return 0;
}
