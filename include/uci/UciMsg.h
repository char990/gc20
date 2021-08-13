#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <uci/UciCfg.h>
/*
Filename: "./config/UciMsg"
Format: UCI
--- Start ---
config UciMsg msg

    # msg_xxx : xxx is message ID, 001-255, 000 is not allowed

    # same as SetMessage, total 20 bytes, unsed part filled with 00 and CRC attached
    option msg_001  "0C010000010102020303040400000000CRC"

    # The last entry is checksum, sum up all messages' CRC
    # when any msg was changed, must update checksum
    option checksum "55AA"

    # If msg CRC is not matched, discard msg
--- End ---
*/

class UciMsg : public UciCfg
{
public:
    UciMsg();
    char const* path = "./config";
    char const* package = "UciMsg";
    char const* config = "msg";

    void LoadConfig() override;
	void Dump() override;

    uint16_t ChkSum();
};

#endif
