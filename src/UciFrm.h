#ifndef __UCIFRM_H__
#define __UCIFRM_H__

// High-res gfx frame
// max size of a 24-bit RGB frame is 288*64*3 = 55296 bytes
// total bytes : 15 + 55296 = 55311
#define MAX_HRGFRM_SIZE 55311

// Gfx frame
// max size of a 4-bit frame is 256*64/2 = 8192 bytes
// total bytes : 11 + 8192 = 8203
#define MAX_GFXFRM_SIZE 8203

// Text frame
// max size of a text frame is 255 bytes
// total bytes : 9 + 255 = 264
#define MAX_TXTFRM_SIZE 264

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
    static const char * filename = "UciFrm";
    UciFrm();

};

#endif
