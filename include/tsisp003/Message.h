#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <cstdint>
#include <string>
#include <tsisp003/TsiSp003Const.h>

struct MsgEntry
{
    uint8_t frmId;
    uint8_t onTime;
};

class Message
{
public:
    /// \breif  ini msg with string, with attached CRC
    Message(char * msg, int len);
    
    /// \breif  ini msg with hex array, no CRC attatched
    Message(uint8_t * msg, int len);
    
    uint8_t appErr;
    uint8_t micode;
    uint8_t msgId;
    uint8_t msgRev;
    uint8_t transTime;
    MsgEntry msgEntries[6];
    uint16_t crc;

    std::string ToString();
};

#endif
