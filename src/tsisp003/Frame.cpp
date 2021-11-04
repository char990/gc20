#include <stdexcept>
#include <cstring>

#include <tsisp003/Frame.h>
#include <module/Utils.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>

using namespace Utils;

Frame::Frame()
{
}

Frame::~Frame()
{
}

int Frame::FrameCheck(uint8_t *frm, int len)
{
    crc = Crc::Crc16_1021(frm, len - 2);
    if (crc != Cnvt::GetU16(frm + len - 2))
    {
        appErr = APP::ERROR::DataChksumError;
        return 1;
    }
    if (frmId == 0)
    {
        appErr = APP::ERROR::SyntaxError;
        return 1;
    }
    if (len != (frmOffset + 2 + frmBytes))
    {
        appErr = APP::ERROR::LengthError;
        return 1;
    }
    if (CheckLength(len) != 0)
    {
        return 1;
    }
    if (CheckSub(frm, len) != 0)
    { // special parameters in Txt/Gfx/Hrg Frame
        return 1;
    }
    if (CheckColour() != 0)
    {
        appErr = APP::ERROR::ColourNotSupported;
        return 1;
    }
    // int CheckConspicuity();
    UciProd &prod = DbHelper::Instance().GetUciProd();
    if ((conspicuity & 0x07) > 5 || ((conspicuity >> 3) & 0x03) > 2 ||
        !prod.IsConspicuity(conspicuity & 0x07) ||
        !prod.IsAnnulus((conspicuity >> 3) & 0x03))
    {
        appErr = APP::ERROR::ConspicuityNotSupported;
        return 1;
    }
    appErr = APP::ERROR::AppNoError;
    return 0;
}

/*****************************FrmTxt*******************************/
FrmTxt::FrmTxt(uint8_t *frm, int len)
{
    frmOffset = TXTFRM_HEADER_SIZE;
    micode = frm[0];
    frmId = frm[1];
    frmRev = frm[2];
    font = frm[3];
    colour = frm[4];
    conspicuity = frm[5];
    frmBytes = frm[6];
    if (Frame::FrameCheck(frm, len) == 0)
    {
        stFrm.Init(frm, len);
    }
}

int FrmTxt::CheckLength(int len)
{
    if (len > (255 + frmOffset + 2))
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

int FrmTxt::CheckSub(uint8_t *frm, int len)
{
    if (micode != static_cast<uint8_t>(MI::CODE::SignSetTextFrame))
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    else if (Check::Text(frm + frmOffset, frmBytes) != 0)
    {
        appErr = APP::ERROR::TextNonASC;
        return 1;
    }
    else if (!DbHelper::Instance().GetUciProd().IsFont(font))
    {
        appErr = APP::ERROR::FontNotSupported;
        return 1;
    }
    /* TODO if text could fit in the sign: X chars * Y chars
    if (???)
    {
        appErr = APP::ERROR::FrameTooLarge;
        return 1;
    }
    */
    return 0;
}

int FrmTxt::CheckColour()
{
    return DbHelper::Instance().GetUciProd().IsTxtFrmColour(colour) ? 0 : -1;
}

std::string FrmTxt::ToString()
{
    char buf[256];
    snprintf(buf, 255, "MI=0x%02X(Txt), Id=%d, Rev=%d, Font=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
             micode, frmId, frmRev, font, colour, conspicuity, frmBytes, crc);
    std::string s(buf);
    return s;
}

/****************************** FrmGfx *******************************/
FrmGfx::FrmGfx(uint8_t *frm, int len)
{
    frmOffset = GFXFRM_HEADER_SIZE;
    micode = frm[0];
    frmId = frm[1];
    frmRev = frm[2];
    pixelRows = frm[3];
    pixelColumns = frm[4];
    colour = frm[5];
    conspicuity = frm[6];
    frmBytes = Cnvt::GetU16(frm + 7);
    if (Frame::FrameCheck(frm, len) == 0)
    {
        stFrm.Init(frm, len);
    }
}

int FrmGfx::CheckSub(uint8_t *frm, int len)
{
    if (micode != static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame))
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    return 0;
}

int FrmGfx::CheckLength(int len)
{
    UciProd &prod = DbHelper::Instance().GetUciProd();
    if (len < frmOffset + 2 + prod.Gfx1FrmLen() ||
        len > frmOffset + 2 + prod.MaxFrmLen())
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
        if (colour < static_cast<uint8_t>(FRMCOLOUR::MonoFinished))
        {
            x = prod.Gfx1FrmLen();
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::MultipleColours))
        {
            x = prod.Gfx4FrmLen();
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::RGB24))
        {
            x = prod.Gfx24FrmLen();
        }
        if (x != 0)
        {
            if (frmBytes > x)
            {
                appErr = APP::ERROR::FrameTooLarge;
            }
            else if (frmBytes < x)
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
    return DbHelper::Instance().GetUciProd().IsGfxFrmColour(colour) ? 0 : -1;
}

std::string FrmGfx::ToString()
{
    char buf[256];
    snprintf(buf, 255, "MI=0x%02X(Gfx), Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
             micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmBytes, crc);
    std::string s(buf);
    return s;
}

/****************************** FrmHrg *******************************/
FrmHrg::FrmHrg(uint8_t *frm, int len)
{
    frmOffset = HRGFRM_HEADER_SIZE;
    micode = frm[0];
    frmId = frm[1];
    frmRev = frm[2];
    pixelRows = Cnvt::GetU16(frm + 3);
    pixelColumns = Cnvt::GetU16(frm + 5);
    colour = frm[7];
    conspicuity = frm[8];
    frmBytes = Cnvt::GetU32(frm + 9);
    if (Frame::FrameCheck(frm, len) == 0)
    {
        stFrm.Init(frm, len);
    }
}

int FrmHrg::CheckSub(uint8_t *frm, int len)
{
    if (micode != static_cast<uint8_t>(MI::CODE::SignSetHighResolutionGraphicsFrame))
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    return 0;
}

int FrmHrg::CheckLength(int len)
{
    UciProd &prod = DbHelper::Instance().GetUciProd();
    if (len < frmOffset + 2 + prod.Gfx1FrmLen() ||
        len > frmOffset + 2 + prod.MaxFrmLen())
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
        if (colour < static_cast<uint8_t>(FRMCOLOUR::MonoFinished))
        {
            x = prod.Gfx1FrmLen();
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::MultipleColours))
        {
            x = prod.Gfx4FrmLen();
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::RGB24))
        {
            x = prod.Gfx24FrmLen();
        }
        if (x != 0)
        {
            if (frmBytes > x)
            {
                appErr = APP::ERROR::FrameTooLarge;
            }
            else if (frmBytes < x)
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
    return DbHelper::Instance().GetUciProd().IsHrgFrmColour(colour) ? 0 : -1;
}

std::string FrmHrg::ToString()
{
    char buf[256];
    snprintf(buf, 255, "MI=0x%02X(Hrg), Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
             micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmBytes, crc);
    std::string s(buf);
    return s;
}
