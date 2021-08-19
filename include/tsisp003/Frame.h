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
    int frmOffset;
    uint8_t * frmData;
    uint16_t crc;

    virtual std::string ToString()=0;

protected:
    /// \brief      Convert asc frame to uint8_t
    /// \param      frm: asc frm
    /// \param      len: asc frm len
    /// \param      min: min frm len(in hex)
    /// \param      max: max frm len(in hex)
    ///             appErr: ref appErr, for convertion result
    void CnvtFrm(char * frm, int len, int min, int max);

    /// \brief Check Conspicuity
    /// \return 0:success
    int CheckConspicuity();

    virtual int CheckColour()=0;
    virtual void CheckLength(int len)=0;
};

class FrmTxt : public Frame
{
public:
    /// \breif  ini frame with string
    FrmTxt(char * frm, int len);
    
    /// \breif  ini frame with hex array
    FrmTxt(uint8_t * frm, int len);
    
    /// \brief delete [] frmData
    ~FrmTxt();

    uint8_t font;

    std::string ToString() override;

private:
    /// \brief Check font
    /// \return 0:success
    int CheckFont();

    /// \brief Check Colour
    /// \return 0:success
    int CheckColour() override;

    /// \brief Check Length, result is in appError
    void CheckLength(int len) override;

    /// \breif make frame
    void MakeFrame(int len);
};

class FrmGfx : public Frame
{
public:
    /// \breif  ini frame with string
    FrmGfx(char * frm, int len);

    /// \breif  ini frame with hex array
    FrmGfx(uint8_t * frm, int len);
    
    /// \brief delete [] frmData
    ~FrmGfx();

    uint8_t pixelRows;
    uint8_t pixelColumns;


    std::string ToString() override;

private:
    /// \brief Check Colour
    /// \return 0:success
    int CheckColour() override;

    /// \brief Check Length, result is in appError
    void CheckLength(int len) override;

    /// \breif make frame
    void MakeFrame(int len);
};

class FrmHrg : public Frame
{
public:
    /// \breif  ini frame with string
    FrmHrg(char * frm, int len);

    /// \breif  ini frame with hex array
    FrmHrg(uint8_t * frm, int len);
    
    /// \brief delete [] frmData
    ~FrmHrg();

    uint16_t pixelRows;
    uint16_t pixelColumns;
    std::string ToString() override;
    
private:
    /// \brief Check Colour
    /// \return 0:success
    int CheckColour() override;

    /// \brief Check Length, result is in appError
    void CheckLength(int len) override;

    /// \breif make frame
    void MakeFrame(int len);
};

#endif
