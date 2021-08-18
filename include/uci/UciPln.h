#ifndef __UCIPLN_H__
#define __UCIPLN_H__

#include <uci/UciCfg.h>
/*
File: "./config/UciPln"
Format: UCI
Note : CRC is uint16_t of HEX
--- Start ---
config UciPln pln

    # pln_xxx : xxx is plan ID, 1-255, 0 is not allowed

    # mostly same as SetPlan, total 42 bytes(4+6*6+2-byte crc) + ":enable_flag", unsed part filled with 00
    // enabled
    option pln_1  "0D01......CRC:1"
    // disabled
    option pln_2  "0D02......CRC:0"

    # If plan CRC is not matched, discard plan
--- End ---
*/

class UciPln : public UciCfg
{
public:
    UciPln();
    char const* PATH = "./config";
    char const* PACKGE = "UciPln";
    char const* SECTION = "pln";

    void LoadConfig() override;
	void Dump() override;

    uint16_t ChkSum();
};

#endif
