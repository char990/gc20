#ifndef __UCIPLANS_H__
#define __UCIPLANS_H__

/*
File: "./UciPln"
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

    # enabled plans, last number is CRC
    option enabled "1,2,3,CRC"

    # If plan CRC is not matched, discard plan
--- End ---
*/

class UciPln
{
    UciPln();
    const char *filename = "UciPln";
};

#endif
