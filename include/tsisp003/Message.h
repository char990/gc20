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

#define MSG_LEN_MAX (4+2*6)
#define MSG_LEN_MIN (4+2*1+1)

class Message
{
public:
    /// \breif  Blank msg
    Message():micode(0),crc(0){};
    ~Message(){};

    /// \breif  ini msg with string, with attached CRC
    APP::ERROR Init(char * msg, int len);
    
    /// \breif  ini msg with hex array, no CRC attatched
    APP::ERROR Init(uint8_t * msg, int len);

    uint8_t micode;
    uint8_t msgId;
    uint8_t msgRev;
    uint8_t transTime;
    MsgEntry msgEntries[6];
    uint16_t crc;

    /// \brief  convert this msg to uint8_t array in format "Sign Set Message"
    /// \param  pbuf : output buf
    /// \return int : length of data
    int ToArray(uint8_t * pbuf);

    /// for Dump()
    std::string ToString();
};

#endif
