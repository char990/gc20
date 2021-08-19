#include <stdexcept>
#include <cstring>

#include <tsisp003/Message.h>

#include <module/Utils.h>

using namespace Utils;

Message::Message(char *cmsg, int clen)
{
    if ((clen&1)==1 || clen < (4+2*1+1+2)*2 || clen > (4+2*6+2)*2) // with crc
    {
        appErr = APP::ERROR::LengthError;
    }
    int xlen=clen/2;
    uint8_t *xmsg = new uint8_t[xlen];
    if (Cnvt::ParseToU8(cmsg, xmsg, clen) != 0)
    {
        appErr = APP::ERROR::SyntaxError;
        return;
    }
    uint16_t c = Crc::Crc16_1021(xmsg, xlen-2);
    if ((c / 0x100) == xmsg[xlen-2] || (c & 0xFF) == xmsg[xlen-1])
    {
        Message(xmsg, xlen-2);
    }
    else
    {
        appErr = APP::ERROR::DataChksumError;
    }
    delete [] xmsg;
}

Message::Message(uint8_t *xmsg, int xlen)
{
    micode = xmsg[0];
    msgId = xmsg[1];
    msgRev = xmsg[2];
    transTime = xmsg[3];
    if (xlen < (4+2*1+1) || xlen > (4+2*6)) // no crc
    {
        appErr = APP::ERROR::LengthError;
    }
    else if (micode != MI::CODE::SignSetMessage)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if (msgId == 0)
    {
        appErr = APP::ERROR::SyntaxError;
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
                    appErr = APP::ERROR::SyntaxError;
                    return;
                }
                break;
            }
            msgEntries[i].onTime = *p++;
        }
        if(p!=xmsg+xlen)
        {
            appErr = APP::ERROR::SyntaxError;
        }
        else
        {
            appErr = APP::ERROR::AppNoError;
            crc = Crc::Crc16_1021(xmsg, xlen);
            msgDatalen = xlen +2;
            msgData = new uint8_t [msgDatalen];
            memcpy(msgData, xmsg, xlen);
            msgData[msgDatalen - 2] = crc/0x100;
            msgData[msgDatalen - 1] = crc;
        }
    }
}

Message::~Message()
{
    if(msgData!=nullptr)
    {
        delete msgData;
    }
}

std::string Message::ToString()
{
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "Message:(appErr=%d) MI=0x%02X, Id=%d, Rev=%d, TransT=%d, ",
                   appErr, micode, msgId, msgRev, transTime);
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
