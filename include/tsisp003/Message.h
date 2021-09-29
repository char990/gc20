#pragma once

#include <cstdint>
#include <string>
#include <tsisp003/TsiSp003Const.h>

struct MsgEntry
{
    uint8_t frmId{0};
    uint8_t onTime{0};
};

#define MSG_LEN_MAX (4+2*6)
#define MSG_LEN_MIN (4+2*1+1)
#define MSG_TAIL 2
class Message
{
public:
    /// \breif  Blank msg
    Message(){};
    ~Message(){};

    /// \breif  ini msg with hex array, last 2-byte is CRC (Don't need to check)
    APP::ERROR Init(uint8_t * msg, int len);

    uint8_t micode{0};
    uint8_t msgId;
    uint8_t msgRev;
    uint8_t transTime;
    uint8_t entries;
    MsgEntry msgEntries[6];
    uint16_t crc{0};

    /// \brief  convert this msg to uint8_t array in format "Sign Set Message" + 2-byte crc
    /// \param  pbuf : output buf
    /// \return int : length of data
    int ToArray(uint8_t * pbuf);

    /// for Dump()
    std::string ToString();

private:
    /// \brief  Check if there is undefined frm in msg
    /// \return -1:msg has no frm; 0:OK; 1: msg has undefined frm 
    int CheckEntries();
};


