#include <stdexcept>
#include <cstring>

#include <tsisp003/Plan.h>

#include <module/Utils.h>

using namespace Utils;

APP::ERROR Plan::Init(char *cpln, int clen)
{
    micode=0;
    if ((clen&1)==1 || clen < (PLN_LEN_MIN+2+1)*2 || clen > (PLN_LEN_MAX+2+1)*2) // with crc & enable flag
    {
        return APP::ERROR::LengthError;
    }
    int xlen=clen/2;
    uint8_t xpln[PLN_LEN_MAX];
    if (Cnvt::ParseToU8(cpln, xpln, clen) != 0 ||
        ( xpln[xlen-1]!=PLN_DISABLED && xpln[xlen-1]!=PLN_ENABLED))
    {
        return APP::ERROR::SyntaxError;
    }
    else
    {
        uint16_t c = Crc::Crc16_1021(xpln, xlen-3);
        if ((c / 0x100) == xpln[xlen-3] || (c & 0xFF) == xpln[xlen-2])
        {
            enabled = xpln[xlen-1];
            return Init(xpln, xlen-3);
        }
        return APP::ERROR::DataChksumError;
    }
}

APP::ERROR Plan::Init(uint8_t *xpln, int xlen)
{
    micode = 0;
    plnId = xpln[1];
    plnRev = xpln[2];
    weekdays = xpln[3];
    if (xlen < PLN_LEN_MIN || xlen > PLN_LEN_MAX) // no crc & enable flag
    {
        return APP::ERROR::LengthError;
    }
    else if (xpln[0] != MI::CODE::SignSetPlan)
    {
        return APP::ERROR::UnknownMi;
    }
    else if (plnId == 0 || (weekdays&0x80)!=0 || (weekdays&0x7F)==0 )
    {
        return APP::ERROR::SyntaxError;
    }
    else
    {
        uint8_t *p = xpln+4;
        for(int i=0;i<6;i++)
        {
            plnEntries[i].type = *p++;
            if(plnEntries[i].type==0)
            {
                if(i==0)
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
        }
        if(p==xpln+xlen)
        {
            micode = xpln[0];
            uint8_t buf[PLN_LEN_MAX];
            int len = ToArray(buf);
            crc = Crc::Crc16_1021(buf, len);
            return APP::ERROR::AppNoError;
        }
        return APP::ERROR::SyntaxError;
    }
}

int Plan::ToArray(uint8_t *pbuf)
{
    uint8_t *p=pbuf;
    *p++=micode;
    *p++=plnId;
    *p++=plnRev;
    *p++=weekdays;
    for(int i=0;i<6;i++)
    {
        *p++=plnEntries[i].type;
        if(plnEntries[i].type==0)
        {
            break;
        }
        *p++=plnEntries[i].fmId;
        *p++=plnEntries[i].start.hour;
        *p++=plnEntries[i].start.min;
        *p++=plnEntries[i].stop.hour;
        *p++=plnEntries[i].stop.min;
    }
    return p-pbuf;
}

std::string Plan::ToString()
{
    if(micode==0)
    {
        return "Plan undefined";
    }
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "Plan: MI=0x%02X, Id=%d, Rev=%d, Weekday=",
                   micode, plnId, plnRev);
    const char * WEEK[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

    for (int i = 0, k=1; i < 7; i++)
    {
        if(k&weekdays)
        {
            len += snprintf(buf + len, 1023 - len, "%s,", WEEK[i]);
        }
        k<<=1;
    }
    len += snprintf(buf + len, 1023 - len, " ");
    for (int i = 0; i < 6; i++)
    {
        if (plnEntries[i].type == 1)
        {
            len += snprintf(buf + len, 1023 - len, "(Frm" );
        }
        else if(plnEntries[i].type == 2)
        {
            len += snprintf(buf + len, 1023 - len, "(Msg" );
        }
        else
        {
            len += snprintf(buf + len, 1023 - len, ", " );
            break;
        }
        len += snprintf(buf + len, 1023 - len, "[%d]%d:%02d-%d:%02d)", plnEntries[i].fmId,
            plnEntries[i].start.hour, plnEntries[i].start.min,
            plnEntries[i].stop.hour, plnEntries[i].stop.min);
    }
    snprintf(buf + len, 1023 - len, "Crc=0x%04X, en/dis=0x%X", crc, enabled);
    std::string s(buf);
    return s;
}
