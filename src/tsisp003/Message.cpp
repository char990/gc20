#include <stdexcept>
#include <cstring>

#include <tsisp003/Message.h>

#include <module/Utils.h>

using namespace Utils;

Message::Message(char *msg, int len)
{
    if (len != 36)
    {
        appErr = APP::ERROR::LengthError;
        return;
    }
    uint8_t msgh[18];
    if (Cnvt::ParseToU8(msg, msgh, 36) != 0)
    {
        appErr = APP::ERROR::SyntaxError;
        return;
    }
    uint16_t c = Crc::Crc16_1021(msgh, 16);
    if ((c / 0x100) == msgh[16] || (c & 0xFF) == msgh[17])
    {
        Message(msgh, 16);
    }
    else
    {
        appErr = APP::ERROR::DataChksumError;
    }
}

Message::Message(uint8_t *msg, int len)
{
    if (len != 16)
    {
        appErr = APP::ERROR::LengthError;
    }
    else if (msg[0] != MI::CODE::SignSetMessage)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if (msg[1] == 0)
    {
        appErr = APP::ERROR::SyntaxError;
    }
    else
    {
        uint8_t *p = msg+4;
        bool frmiD0=false;
        for(int i=0;i<6;i++)
        {
            msgEntries[i].frmId = *p++;
            msgEntries[i].onTime = *p++;
            if(msgEntries[i].frmId==0)
            {
                frmiD0=true;
            }
            else
            {
                if(frmiD0)
                {
                    appErr = APP::ERROR::SyntaxError;
                    return;
                }
            }
        }
        crc = Crc::Crc16_1021(msg, 16);
        appErr = APP::ERROR::AppNoError;
        micode = msg[0];
        msgId = msg[1];
        msgRev = msg[2];
        transTime = msg[3];
    }
}

std::string Message::ToString()
{
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "Message:(appErr=%d) MI=0x%02X, Id=%d, Rev=%d, TransT=%d, ",
                   appErr, micode, msgId, msgRev, transTime, crc);
    for (int i = 0; i < 6; i++)
    {
        if (msgEntries[i].frmId == 0)
        {
            len = snprintf(buf+len, 1023-len, ", ";
            break;
        }
        len = snprintf(buf + len, 1023 - len, "(%d,%d)", i + 1, msgEntries[i].frmId, msgEntries[i].onTime);
    }
    snprintf(buf + len, 1023 - len, "Crc=0x%04X", crc);
    std::string s(buf);
    return s;
}
