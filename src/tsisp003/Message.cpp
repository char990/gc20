#include <stdexcept>
#include <cstring>

#include <tsisp003/Message.h>

#include <module/Utils.h>
#include <uci/DbHelper.h>

using namespace Utils;

APP::ERROR Message::Init(uint8_t *xmsg, int xlen)
{
    micode = 0;
    msgId = xmsg[1];
    msgRev = xmsg[2];
    transTime = xmsg[3];
    crc = Cnvt::GetU16(xmsg + xlen - MSG_TAIL);
    if (xmsg[0] != MI::CODE::SignSetMessage)
    {
        return APP::ERROR::UnknownMi;
    }
    if (xlen < (MSG_LEN_MIN + MSG_TAIL) || xlen > (MSG_LEN_MAX + MSG_TAIL)) // with crc
    {
        return APP::ERROR::LengthError;
    }
    if (msgId == 0)
    {
        return APP::ERROR::SyntaxError;
    }
    uint8_t *p = xmsg + 4;
    entries = 0;
    for (int i = 0; i < 6; i++)
    {
        msgEntries[i].frmId = *p++;
        if (msgEntries[i].frmId == 0)
        {
            if (i == 0)
            {
                return APP::ERROR::SyntaxError;
            }
            break;
        }
        msgEntries[i].onTime = *p++;
        entries++;
    }
    if (p != (xmsg + xlen - MSG_TAIL))
    {
        return APP::ERROR::SyntaxError;
    }
    if (0 != CheckEntries())
    {
        return APP::ERROR::FrmMsgPlnUndefined;
    }
    micode = MI::CODE::SignSetMessage;
    return APP::ERROR::AppNoError;
}

int Message::ToArray(uint8_t *pbuf)
{
    uint8_t *p = pbuf;
    *p++ = micode;
    *p++ = msgId;
    *p++ = msgRev;
    *p++ = transTime;
    for (int i = 0; i < 6; i++)
    {
        *p++ = msgEntries[i].frmId;
        if (msgEntries[i].frmId == 0)
        {
            break;
        }
        *p++ = msgEntries[i].onTime;
    }
    p = Cnvt::PutU16(crc, p);
    return p - pbuf;
}

std::string Message::ToString()
{
    if (micode == 0)
    {
        return "Message undefined";
    }
    char buf[1024];
    int len = 0;
    len = snprintf(buf, 1023, "msg_%03d: MI=0x%02X, Id=%d, Rev=%d, TransT=%d, Entries(%d)=",
                msgId, micode, msgId, msgRev, transTime, entries);
    for (int i = 0; i < entries; i++)
    {
        len += snprintf(buf + len, 1023 - len, "(%d,%d)", msgEntries[i].frmId, msgEntries[i].onTime);
    }
    snprintf(buf + len, 1023 - len, ", Crc=0x%04X", crc);
    std::string s(buf);
    return s;
}

int Message::CheckEntries()
{
    for (int i = 0; i < entries; i++)
    {
        if (!DbHelper::Instance().uciFrm.IsFrmDefined(msgEntries[i].frmId))
        {
            return -1;
        }
    }
    return 0;
}
