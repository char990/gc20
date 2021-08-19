#ifndef __UCIFRM_H__
#define __UCIFRM_H__

#include <string>
#include <uci/UciCfg.h>
#include <tsisp003/Frame.h>

/*
Filename: "./config/UciFrm"
Format: UCI
--- Start ---
config UciFrm frm

    # frm_xxx : xxx is frame ID, 1-255, 0 is not allowed
    # content of a frame is the app layer data

    # frm_001 is text frame, same as SetTextFrame
    option frm_1  "0A01......"

    # frm_100 is graphics frame, same as SetGfxFrame except for 'bitmap data'
    # 'graphics frame data' saved in file:'frm_100'
    option frm_100  "0B64......"

    # frm_101 is High-res graphics frame, same as SetHrGfxFrame except for 'bitmap data'
    # 'hr graphics frame data' saved in file:'frm_101'
    option frm_101  "1D65......"

    # If frame CRC is not matched, discard frame
--- End ---
*/
class UciFrm : public UciCfg
{
public:
    UciFrm();
    ~UciFrm();
    char const* PATH = "./config";
    char const* PACKAGE = "UciFrm";
    char const* SECTION = "frm";

    /// \brief  load frms[] from "UciFrm"
    void LoadConfig() override;

	void Dump() override;

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get frms[i], check frm->micode to get type
    Frame * GetFrm(int i);

    /// \brief  Set a frame from hex array, e.g. app layer data of SighSetTextFrame
    ///         frame will be stored in frms[] (but not saved in "UciFrm")
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP::ERROR
    uint8_t SetFrm(uint8_t * buf, int len);

    /// \brief  Save frms[i] to "UciFrm"
    ///         When TsiSp003 set a frame, call SetFrm then SaveFrm
    /// \param  i: frms index
    void SaveFrm(int i);

    void TestSaveTxtFrm();
private:
    Frame * frms[256];  // [0] is nullptr
    uint16_t chksum;
};

#endif
