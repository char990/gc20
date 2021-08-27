#ifndef __UCIMSG_H__
#define __UCIMSG_H__

#include <string>
#include <tsisp003/Message.h>
#include <uci/UciCfg.h>
#include <uci/UciFrm.h>

/*
Filename: "./config/UciMsg"
Format: UCI
--- Start ---
config UciMsg msg

    # msg_xxx : xxx is message ID, 1-255, 0 is not allowed

    # mostly same as SetMessage
    # min = 7(4+2*1+1) + crc(2)
    # max = 16(4+2*6) + crc(2)
    option msg_1  "0C01000A011E021E031E041E051E0055AA"

    # If msg CRC is not matched, discard msg
--- End ---
*/

class UciMsg : public UciCfg
{
public:
    UciMsg(UciFrm &uciFrm);
    ~UciMsg();

	const char * SECTION;

    /// \brief  load msgs[] from "UciMsg"
    void LoadConfig() override;

	void Dump() override;

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get msgs[i]
    Message * GetMsg(int i);

    /// \brief  Set a msg from hex array, e.g. app layer data of SighSetMessage
    ///         frame will be stored in msgs[] (but not saved in "UciMsg")
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP::ERROR
    uint8_t SetMsg(uint8_t * buf, int len);

    /// \brief  Save msgs[i] to "UciMsg", with CRC attached
    ///         When TsiSp003 set a msg, call SetMsg then SaveMsg
    /// \param  i: msgs index
    void SaveMsg(int i);


    /// \brief  Check if there is undefined frm in msg
    /// \return -1:msg has no frm; 0:OK; 1: msg has undefined frm 
    int CheckMsgEntries(Message  * msg);

private:
    Message * msgs[256];  // [0] is nullptr
    uint16_t chksum;
    UciFrm &uciFrm;
};

#endif
