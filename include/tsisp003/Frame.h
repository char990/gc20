#ifndef __FRAME_H__
#define __FRAME_H__

#include <cstdint>
#include <string>
#include <tsisp003/TsiSp003Const.h>


class Frame
{
public:
    Frame();
    virtual ~Frame(){};
    uint8_t appErr;
    uint8_t micode;
    uint8_t frmId;
    uint8_t frmRev;
    uint8_t colour;
    uint8_t conspicuity;
    int frmlen;
    uint8_t * frmData;
    uint16_t crc;

    /// \brief      Convert asc frame to uint8_t
    /// \param      frm: asc frm
    /// \param      len: asc frm len
    /// \param      max: max frm len(in hex)
    ///             appErr: ref appErr, for convertion result
    void CnvtFrm(char * frm, int len, int max);

    /// \brief Check Conspicuity
    /// \return 0:success
    int CheckConspicuity(uint8_t v);

    virtual std::string ToString()=0;
};

class FrmTxt : public Frame
{
public:
    /// \breif  ini frame with string
    FrmTxt(char * frm, int len);
    
    /// \breif  ini frame with hex array
    FrmTxt(uint8_t * frm, int len);
    
    /// \breif make frame
    void Frame(int len);

    /// \brief delete [] frmData
    ~FrmTxt();

    uint8_t font;

    /// \brief Check font
    /// \return 0:success
    int CheckFont(uint8_t font);

    /// \brief Check Colour
    /// \return 0:success
    int CheckColour(uint8_t font);

    std::string ToString() override;
};

class FrmGfx : public Frame
{
public:
    FrmGfx(char * frm, int len);
    FrmGfx(uint8_t * frm, int len);
    ~FrmGfx();
    uint8_t pixelRows;
    uint8_t pixelColumns;
    std::string ToString() override;
};

class FrmHrg : public Frame
{
public:
    FrmHrg(char * frm, int len);
    FrmHrg(uint8_t * frm, int len);
    ~FrmHrg();
    uint16_t pixelRows;
    uint16_t pixelColumns;
    std::string ToString() override;
};

#endif
