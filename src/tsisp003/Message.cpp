#include <stdexcept>
#include <cstring>

#include <tsisp003/Message.h>

#include <module/Utils.h>

using namespace Utils;

APP::ERROR Message::Init(char *cmsg, int clen)
{
    micode=0;
    if ((clen&1)==1 || clen < (MSG_LEN_MIN+2)*2 || clen > (MSG_LEN_MAX+2)*2) // with crc
    {
        return APP::ERROR::LengthError;
    }
    int xlen=clen/2;
    uint8_t xmsg[MSG_LEN_MAX+2];
    if (Cnvt::ParseToU8(cmsg, xmsg, clen) != 0)
    {
        return APP::ERROR::SyntaxError;
    }
    uint16_t c = Crc::Crc16_1021(xmsg, xlen-2);
    if ((c / 0x100) == xmsg[xlen-2] || (c & 0xFF) == xmsg[xlen-1])
    {
        return Init(xmsg, xlen-2);
    }
    return APP::ERROR::DataChksumError;
}

APP::ERROR Message::Init(uint8_t *xmsg, int xlen)
{
    micode=0;
    msgId = xmsg[1];
    msgRev = xmsg[2];
    transTime = xmsg[3];
    if (xlen < MSG_LEN_MIN || xlen > MSG_LEN_MAX) // no crc
    {
        return APP::ERROR::LengthError;
    }
    else if (xmsg[0] != MI::CODE::SignSetMessage)
    {
        return APP::ERROR::UnknownMi;
    }
    else if (msgId == 0)
    {
        return APP::ERROR::SyntaxError;
    }
    else
    {
        uint8_t *p = xmsg+4;
        for(int i=0;i<6;i++)
        {
            msgEntries[i].frmId = *p++;
            if(msgEntries[i].frmId==0)
            {
                if(i==0)
                {
                    return APP::ERROR::SyntaxError;
                }
                break;
            }
            msgEntries[i].onTime = *p++;
        }
        if(p==xmsg+xlen)
        {
            micode = xmsg[0];
            uint8_t buf[MSG_LEN_MAX];
            int len = ToArray(buf);
            crc = Crc::Crc16_1021(buf, len);
            return APP::ERROR::AppNoError;
        }
        return APP::ERROR::SyntaxError;
    }
}

int Message::ToArray(uint8_t *pbuf)
{
    uint8_t *p=pbuf;
    *p++=micode;
    *p++=msgId;
    *p++=msgRev;
    *p++=transTime;
    for(int i=0;i<6;i++)
    {
        *p++=msgEntries[i].frmId;
        if(msgEntries[i].frmId==0)
        {
            break;
        }
        *p++=msgEntries[i].onTime;
    }
    return p-pbuf;
}

std::string Message::ToString()
{
    if(micode==0)
    {
        return "Message undefined";
    }
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "Message: MI=0x%02X, Id=%d, Rev=%d, TransT=%d, ",
                   micode, msgId, msgRev, transTime);
    for (int i = 0; i < 6; i++)
    {
        if (msgEntries[i].frmId == 0)
        {
            len += snprintf(buf+len, 1023-len, ", ");
            break;
        }
        len += snprintf(buf + len, 1023 - len, "(%d,%d)", i + 1, msgEntries[i].frmId, msgEntries[i].onTime);
    }
    snprintf(buf + len, 1023 - len, "Crc=0x%04X", crc);
    std::string s(buf);
    return s;
}
