#ifndef __UCIPLANS_H__
#define __UCIPLANS_H__

#include <uci/UciCfg.h>
/*
File: "./config/UciPln"
Format: UCI
Note : CRC is uint16_t of HEX
--- Start ---
config UciPln pln

    # pln_xxx : xxx is plan ID, 001-255, 000 is not allowed

    # same as SetPlan, total 42 bytes, unsed part filled with 00 and CRC attached
    option pln_001  "0D01......CRC"

    # sum up all plans' CRC
    # when any plan was changed, must update checksum
    option checksum "55AA"

    # enabled plans
    option enabled_plans "1,2,3"

    # If plan CRC is not matched, discard plan
--- End ---
*/

class UciPln : public UciCfg
{
public:
    UciPln();
    char const* path = "./config";
    char const* package = "UciPln";
    char const* config = "pln";

    void LoadConfig() override;
	void Dump() override;

    uint16_t ChkSum();
};

#endif
