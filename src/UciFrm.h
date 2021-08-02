#ifndef __UCIFRM_H__
#define __UCIFRM_H__



/*
Filename: "./UciFrm"
Format: UCI
--- Start ---
config UciFrm frm

    # frm_xxx : xxx is frame ID, 001-255, 000 is not allowed

    # frm_001 is text frame, same as SetTextFrame
    option frm_001  "0A01......CRC"

    # frm_100 is graphics frame, same as SetGfxFrame excepte for 'graphics frame data'
    # 'graphics frame data' saved in file:'frm_100'
    option frm_100  "0B64......CRC"

    # frm_101 is High-res graphics frame, same as SetHrGfxFrame excepte for 'hr graphics frame data'
    # 'hr graphics frame data' saved in file:'frm_101'
    option frm_101  "1D65......CRC"

    # The last entry is checksum, sum up all frames' CRC
    # when any frame was changed, must update checksum
    option checksum "55AA"

    # If frame CRC is not matched, discard frame
--- End ---
*/




class UciFrm
{
public:
    UciFrm();
    const char * filename = "UciFrm";

};

#endif
