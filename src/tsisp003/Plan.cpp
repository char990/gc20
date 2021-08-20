#include <stdexcept>
#include <cstring>

#include <tsisp003/Plan.h>

#include <module/Utils.h>

using namespace Utils;

Plan::Plan(char *cpln, int clen)
{
    if ((clen&1)==1 || clen < (4+6*1+1+2+1)*2 || clen > (4+6*6+2+1)*2) // with crc & enable flag
    {
        appErr = APP::ERROR::LengthError;
    }
    int xlen=clen/2;
    uint8_t *xpln = new uint8_t[xlen];
    if (Cnvt::ParseToU8(cpln, xpln, clen) != 0)
    {
        appErr = APP::ERROR::SyntaxError;
    }
    else if( xpln[xlen-1]!=0x50 && xpln[xlen-1]!=0x51)
    {
        appErr = APP::ERROR::SyntaxError;
    }
    else
    {
        uint16_t c = Crc::Crc16_1021(xpln, xlen-3);
        if ((c / 0x100) == xpln[xlen-3] || (c & 0xFF) == xpln[xlen-2])
        {
            enabled = xpln[xlen-1];
            Plan(xpln, xlen-3);
        }
        else
        {
            appErr = APP::ERROR::DataChksumError;
        }
    }
    delete [] xpln;
}

Plan::Plan(uint8_t *xpln, int xlen)
{
    micode = xpln[0];
    plnId = xpln[1];
    plnRev = xpln[2];
    weekdays = xpln[3];
    if (xlen < (4+6*1+1) || xlen > (4+6*6)) // no crc & enable flag
    {
        appErr = APP::ERROR::LengthError;
    }
    else if (micode != MI::CODE::SignSetPlan)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if (plnId == 0 || (weekdays&0x80)!=0)
    {
        appErr = APP::ERROR::SyntaxError;
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
                    appErr = APP::ERROR::SyntaxError;
                    return;
                }
                break;
            }
            plnEntries[i].fmId = *p++;
            plnEntries[i].start.hour = *p++;
            plnEntries[i].start.min = *p++;
            plnEntries[i].stop.hour = *p++;
            plnEntries[i].stop.min = *p++;
        }
        if(p!=xpln+xlen)
        {
            appErr = APP::ERROR::SyntaxError;
        }
        else
        {
            appErr = APP::ERROR::AppNoError;
            crc = Crc::Crc16_1021(xpln, xlen);
            plnDataLen = xlen +2;
            plnData = new uint8_t [plnDataLen];
            memcpy(plnData, xpln, xlen);
            plnData[plnDataLen - 2] = crc/0x100;
            plnData[plnDataLen - 1] = crc;
        }
    }
}

std::string Plan::ToString()
{
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "Plan:(appErr=%d) MI=0x%02X, Id=%d, Rev=%d, Weekday=",
                   appErr, micode, plnId, plnRev);
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
            break;
        }
        len += snprintf(buf + len, 1023 - len, "[%d]%d:%02d-%d:%02d)", plnEntries[i].fmId,
            plnEntries[i].start.hour, plnEntries[i].start.min,
            plnEntries[i].stop.hour, plnEntries[i].stop.min);
    }
    snprintf(buf + len, 1023 - len, ", Crc=0x%04X, en/dis=%x", crc, enabled);
    std::string s(buf);
    return s;
}
