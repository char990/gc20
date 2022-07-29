#pragma once

#include <string>
#include <array>
#include <uci/UciProd.h>
#include <tsisp003/Frame.h>
#include <module/Utils.h>

/*
Filename: "./config/frm_xxx"
frm_xxx : xxx is frame ID, 001-255, 0 is not allowed
file content is same as SetTextFrame/SetGfxFrame/SetHrGfxFrame
E.g.
    frm_001:TextFrame
        0A01......
    frm_100:GfxFrame
        0B64......
    frm_101:HrGfxFrame
        1D65......
If frame CRC is "0000", ignore it and make a real crc to replace it
 or
If frame CRC is not matched, discard frame
--- End ---
*/
class UciFrm
{
public:
    UciFrm();
    ~UciFrm();

    /// \brief  load frms & mapped_frms from "UciFrm"
    void LoadConfig();

    void LoadFrms(const char *FMT);

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get frms[i]->stFrm
    StFrm *GetStFrm(uint8_t i);

    /// \brief  Get frms[i]
    Frame *GetFrm(uint8_t i);

    bool IsFrmDefined(uint8_t i);

    /// \brief  Get stFrms[i]->frmRev
    uint8_t GetFrmRev(uint8_t i);

    /// \brief  Set a frame from hex array, e.g. app layer data of SighSetTextFrame
    ///         frame will be stored in stFrms[] (but not saved in "UciFrm")
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP::ERROR
    APP::ERROR SetFrm(uint8_t *buf, int len);

    /// \brief  Save stFrms[i] to "UciFrm" and frame_xxx.bmp
    ///         When TsiSp003 set a frame, call SetFrm then SaveFrm
    /// \param  i: frm id, 0 is NOT valid
    void SaveFrm(uint8_t i);

    /// \brief  delete frms[0] and set frms[0] as nullptr. This is for SignTest
    void DeleteFrm0()
    {
        if (frms[0] != nullptr)
        {
            delete frms[0];
            frms[0] = nullptr;
        }
    }

    void Reset();

    bool IsFrmFlashing(uint8_t i);

private:
    const char *PATH;
    int maxFrmSize{0};
    uint16_t chksum{0};
    std::array<Frame *, 256> frms;
    void Dump();
};
