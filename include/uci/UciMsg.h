#pragma once

#include <string>
#include <array>
#include <tsisp003/Message.h>
#include <uci/UciCfg.h>
#include <uci/UciFrm.h>

/*
Filename: "./config/UciMsg"
Format: UCI
--- Start ---
config UciMsg msg

    # msg_xxx : xxx is message ID, 001-255, 0 is not allowed

    # mostly same as SetMessage
    # min = 7(4+2*1+1) + crc(2)
    # max = 16(4+2*6) + crc(2)
    option msg_001  "0C01000A011E021E031E041E051E0055AA"

    # If msg CRC is not matched, discard msg
--- End ---
*/

class UciMsg : public UciCfg
{
public:
    /// \brief  load msgs[] from "UciMsg"
    void LoadConfig() override;

	void Dump() override;

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get msgs[i-1]
    Message * GetMsg(uint8_t i);

    bool IsMsgDefined(uint8_t i);
    
    /// \brief  Get msgs[i-1]->msgRev, msgs[0] is 0
    uint8_t GetMsgRev(uint8_t i);

    /// \brief  Set a msg from hex array, e.g. app layer data of SighSetMessage
    ///         frame will be stored in msgs[] (but not saved in "UciMsg")
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP::ERROR
    APP::ERROR SetMsg(uint8_t * buf, int len);

    /// \brief  Save msgs[i-1] to "UciMsg", with CRC attached
    ///         When TsiSp003 set a msg, call SetMsg then SaveMsg
    /// \param  i: msgs index
    void SaveMsg(uint8_t i);

    void Reset();

    bool IsMsgFlashing(uint8_t i);
    
private:
    std::array<Message, 255> msgs;  // 255 msgs
    uint16_t chksum;
    const char * _Option = "msg_%03d";
};

