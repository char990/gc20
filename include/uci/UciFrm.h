#pragma once


#include <string>
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
If frame CRC is not matched, discard frame
--- End ---
*/
class UciFrm
{
public:
    UciFrm();
    ~UciFrm();

    /// \brief  load stFrms[] from "UciFrm"
    void LoadConfig();

    /// \brief  Sum all frames's crc
    uint16_t ChkSum();

    /// \brief  Get frms[i-1]->stFrm, frms[0]/undefined frm return nullptr
    StFrm* GetStFrm(uint8_t i);

    /// \brief  Get frms[i-1], check frm->micode to get type, frms[0] is nullptr
    Frame* GetFrm(uint8_t i);

    bool IsFrmDefined(uint8_t i);

    /// \brief  Get stFrms[i-1]->frmRev, stFrms[0] is 0
    uint8_t GetFrmRev(uint8_t i);

    /// \brief  Set a frame from hex array, e.g. app layer data of SighSetTextFrame
    ///         frame will be stored in stFrms[] (but not saved in "UciFrm")
    /// \param  buf: hex array
    /// \param  len: array length
    /// \return APP_ERROR
    APP_ERROR SetFrm(uint8_t * buf, int len);

    /// \brief  Save stFrms[i-1] to "UciFrm"
    ///         When TsiSp003 set a frame, call SetFrm then SaveFrm
    /// \param  i: frm id
    void SaveFrm(uint8_t i);


    void Reset();

    bool IsFrmFlashing(uint8_t i);

private:
    const char * PATH;
    int maxFrmSize{0};
    uint16_t chksum{0};
    Frame *frms[255];
    void Dump();
};

