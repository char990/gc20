#include <stdexcept>
#include <cstring>

#include <tsisp003/Frame.h>
#include <module/Utils.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>

using namespace Utils;

Frame::Frame()
    : micode(0),appErr(APP::ERROR::AppNoError)
{
}

Frame::~Frame()
{
}

void Frame::FrameCheck()
{
    uint8_t *frm = stFrm.rawData;
    int len = stFrm.dataLen;
    crc = Crc::Crc16_1021(frm, len - 2);
    if (crc != Cnvt::GetU16(frm + len - 2))
    {
        appErr = APP::ERROR::DataChksumError;
        return;
    }
    if (frmId == 0)
    {
        appErr = APP::ERROR::SyntaxError;
        return;
    }
    if (len != (frmOffset + 2 + frmlen))
    {
        appErr = APP::ERROR::LengthError;
        return;
    }
    if (CheckLength(len) != 0)
    {
        return;
    }
    if (CheckSub() != 0)
    {
        return;
    }
    if (CheckColour() != 0)
    {
        appErr = APP::ERROR::ColourNotSupported;
        return;
    }
    // int CheckConspicuity();
    if ( (conspicuity & 0x07) > 5 || ((conspicuity >> 3) & 0x03)>2 ||
        !DbHelper::Instance().uciProd.IsConspicuity(conspicuity & 0x07) ||
        !DbHelper::Instance().uciProd.IsAnnulus((conspicuity >> 3) & 0x03))
    {
        appErr = APP::ERROR::ConspicuityNotSupported;
    }
}

/*****************************FrmTxt*******************************/
FrmTxt::FrmTxt(uint8_t *frm, int len)
{
    frmOffset = TXTFRM_HEADER_SIZE;
    stFrm.rawData = frm;
    stFrm.dataLen = len;
    micode = frm[0];
    frmId = frm[1];
    frmRev = frm[2];
    font = frm[3];
    colour = frm[4];
    conspicuity = frm[5];
    frmlen = frm[6];
    Frame::FrameCheck();
}

int FrmTxt::CheckLength(int len)
{
    if (len > (255 + frmOffset + 2) || 0 /* if text could fit in the sign: X chars * Y chars*/)
    {
        appErr = APP::ERROR::FrameTooLarge;
        return 1;
    }
    else if (len < (frmOffset + 2 + 1))
    {
        appErr = APP::ERROR::FrameTooSmall;
        return 1;
    }
    return 0;
}

int FrmTxt::CheckSub()
{
    if (micode != MI::CODE::SignSetTextFrame)
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    else if (Check::Text(stFrm.rawData + frmOffset, frmlen) != 0)
    {
        appErr = APP::ERROR::TextNonASC;
        return 1;
    }
    else if (!DbHelper::Instance().uciProd.IsFont(font))
    {
        appErr = APP::ERROR::FontNotSupported;
        return 1;
    }
    return 0;
}

int FrmTxt::CheckColour()
{
    return DbHelper::Instance().uciProd.IsTxtFrmColour(colour) ? 0 : -1 ;
}

std::string FrmTxt::ToString()
{
    char buf[1024];
    snprintf(buf, 1023, "MI=0x%02X(Txt), Id=%d, Rev=%d, Font=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
            micode, frmId, frmRev, font, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}

/****************************** FrmGfx *******************************/
FrmGfx::FrmGfx(uint8_t *frm, int len)
{
    frmOffset = GFXFRM_HEADER_SIZE;
    stFrm.rawData = frm;
    stFrm.dataLen = len;
    micode = frm[0];
    frmId = frm[1];
    frmRev = frm[2];
    pixelRows = frm[3];
    pixelColumns = frm[4];
    colour = frm[5];
    conspicuity = frm[6];
    frmlen = Cnvt::GetU16(frm + 7);
    Frame::FrameCheck();
}

int FrmGfx::CheckSub()
{
    if (micode != MI::CODE::SignSetGraphicsFrame)
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    return 0;
}

int FrmGfx::CheckLength(int len)
{
    UciProd &prod = DbHelper::Instance().uciProd;
    if (len < frmOffset + 2 + prod.Gfx1FrmLen() ||
        len > frmOffset + 2 + prod.MaxFrmLen() )
    {
        appErr = APP::ERROR::LengthError;
    }
    else if (pixelRows != prod.PixelRows() || pixelColumns != prod.PixelColumns()) // rows & columns
    {
        appErr = APP::ERROR::SizeMismatch;
    }
    else
    {
        int x = 0;
        if (colour < FRM::COLOUR::MonoFinished)
        {
            x = prod.Gfx1FrmLen();
        }
        else if (colour == FRM::COLOUR::MultipleColours)
        {
            x = prod.Gfx4FrmLen();
        }
        else if (colour == FRM::COLOUR::RGB24)
        {
            x = prod.Gfx24FrmLen();
        }
        if (x != 0)
        {
            if (frmlen > x)
            {
                appErr = APP::ERROR::FrameTooLarge;
            }
            else if (frmlen < x)
            {
                appErr = APP::ERROR::FrameTooSmall;
            }
        }
        else
        {
            appErr = APP::ERROR::ColourNotSupported;
        }
    }
    return (appErr == APP::ERROR::AppNoError) ? 0 : -1;
}

int FrmGfx::CheckColour()
{
    return DbHelper::Instance().uciProd.IsGfxFrmColour(colour) ? 0 : -1 ;
}

std::string FrmGfx::ToString()
{
    char buf[1024];
    snprintf(buf, 1023, "MI=0x%02X(Gfx), Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
            micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}

/****************************** FrmHrg *******************************/
FrmHrg::FrmHrg(uint8_t *frm, int len)
{
    frmOffset = HRGFRM_HEADER_SIZE;
    stFrm.rawData = frm;
    stFrm.dataLen = len;
    micode = frm[0];
    frmId = frm[1];
    frmRev = frm[2];
    pixelRows = Cnvt::GetU16(frm + 3);
    pixelColumns = Cnvt::GetU16(frm + 5);
    colour = frm[7];
    conspicuity = frm[8];
    frmlen = Cnvt::GetU32(frm + 9);
    Frame::FrameCheck();
}

int FrmHrg::CheckSub()
{
    if (micode != MI::CODE::SignSetHighResolutionGraphicsFrame)
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    return 0;
}

int FrmHrg::CheckLength(int len)
{
    UciProd &prod = DbHelper::Instance().uciProd;
    if (len < frmOffset + 2 + prod.Gfx1FrmLen() ||
        len > frmOffset + 2 + prod.MaxFrmLen() )
    {
        appErr = APP::ERROR::LengthError;
    }
    else if (pixelRows != prod.PixelRows() || pixelColumns != prod.PixelColumns()) // rows & columns
    {
        appErr = APP::ERROR::SizeMismatch;
    }
    else
    {
        int x = 0;
        if (colour < FRM::COLOUR::MonoFinished)
        {
            x = prod.Gfx1FrmLen();
        }
        else if (colour == FRM::COLOUR::MultipleColours)
        {
            x = prod.Gfx4FrmLen();
        }
        else if (colour == FRM::COLOUR::RGB24)
        {
            x = prod.Gfx24FrmLen();
        }
        if (x != 0)
        {
            if (frmlen > x)
            {
                appErr = APP::ERROR::FrameTooLarge;
            }
            else if (frmlen < x)
            {
                appErr = APP::ERROR::FrameTooSmall;
            }
        }
        else
        {
            appErr = APP::ERROR::ColourNotSupported;
        }
    }
    return (appErr == APP::ERROR::AppNoError) ? 0 : -1;
}

int FrmHrg::CheckColour()
{
    return DbHelper::Instance().uciProd.IsHrgFrmColour(colour) ? 0 : -1 ;
}

std::string FrmHrg::ToString()
{
    char buf[1024];
    snprintf(buf, 1023, "MI=0x%02X(Hrg), Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
            micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}
