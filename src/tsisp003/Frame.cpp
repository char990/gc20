#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>

#include <tsisp003/Frame.h>
#include <module/Utils.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

using namespace Utils;
using namespace std;

Frame::Frame()
{
}

Frame::~Frame()
{
}

int Frame::FrameCheck(uint8_t *frm, int len)
{
    crc = Crc::Crc16_1021(frm, len - 2);
    auto crc2 = Cnvt::GetU16(frm + len - 2);
    if (crc != crc2)
    {
        Ldebug("Frame[%d] Error:Crc mismatch:%04X:%04X", frmId, crc, crc2);
        appErr = APP::ERROR::DataChksumError;
        return 1;
    }
    if (len != (frmOffset + 2 + frmBytes))
    {
        Ldebug("Frame[%d] Error:length mismatch:%d:%d", frmId, len, (frmOffset + 2 + frmBytes));
        appErr = APP::ERROR::LengthError;
        return 1;
    }
    if (CheckColour() != 0)
    {
        appErr = APP::ERROR::ColourNotSupported;
        return 1;
    }
    if (CheckSub(frm, len) != 0)
    { // special parameters in Txt/Gfx/Hrg Frame
        return 1;
    }
    // int CheckConspicuity();
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    auto cons = GetConspicuity(conspicuity);
    auto annu = GetAnnulus(conspicuity);
    if (cons > 5 || annu > 2 ||
        !ucihw.IsConspicuity(cons) ||
        !ucihw.IsAnnulus(annu))
    {
        Ldebug("Frame[%d] Error:conspicuity=%d;annulus=%d", frmId, cons, annu);
        appErr = APP::ERROR::ConspicuityNotSupported;
        return 1;
    }
    appErr = APP::ERROR::AppNoError;
    return 0;
}

int Frame::CheckLength(int len)
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    if (len < frmOffset + 2 + ucihw.Gfx1CoreLen())
    {
        Ldebug("Frame[%d] Error:len=%d", frmId, len);
        appErr = APP::ERROR::FrameTooSmall;
    }
    else if (len > frmOffset + 2 + ucihw.MaxCoreLen())
    {
        Ldebug("Frame[%d] Error:len=%d", frmId, len);
        appErr = APP::ERROR::FrameTooLarge;
    }
    else if (pixelRows != ucihw.PixelRows() || pixelColumns != ucihw.PixelColumns()) // rows & columns
    {
        Ldebug("Frame[%d] Error:pixelRows=%d;pixelColumns=%d", frmId, pixelRows, pixelColumns);
        appErr = APP::ERROR::SizeMismatch;
    }
    else
    {
        int x = 0;
        if (colour < static_cast<uint8_t>(FRMCOLOUR::MonoFinished))
        {
            x = ucihw.Gfx1CoreLen();
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::Multi_4bit))
        {
            x = ucihw.Gfx4CoreLen();
        }
        else if (colour == static_cast<uint8_t>(FRMCOLOUR::RGB_24bit))
        {
            x = ucihw.Gfx24CoreLen();
        }
        if (x != 0)
        {
            if (frmBytes != x)
            {
                Ldebug("Frame[%d] Error:frmBytes mismatch:%d:%d\n", frmId, frmBytes, x);
                appErr = (frmBytes > x) ? APP::ERROR::FrameTooLarge : APP::ERROR::FrameTooSmall;
            }
        }
        else
        {
            Ldebug("Frame[%d] Error:colour=%d", frmId, colour);
            appErr = APP::ERROR::ColourNotSupported;
        }
    }
    return (appErr == APP::ERROR::AppNoError) ? 0 : -1;
}

int Frame::CheckMultiColour(uint8_t *frm, int len)
{
    if (colour == static_cast<uint8_t>(FRMCOLOUR::Multi_4bit))
    {
        auto p = frm + frmOffset;
        auto &ucihw = DbHelper::Instance().GetUciHardware();
        auto monoFinished = static_cast<uint8_t>(FRMCOLOUR::MonoFinished);
        for (int i = 0; i < frmBytes; i++)
        {
            auto d = *p++;
            for (int j = 0; j < 2; j++, d >>= 4)
            {
                uint8_t d2 = d & 0x0F;
                if (d2 == 0)
                {
                    continue;
                }
                if (d2 >= monoFinished || !ucihw.IsGfxFrmColourValid(d2))
                {
                    appErr = APP::ERROR::ColourNotSupported;
                    Ldebug("Frame[%d] Error:MultipleColours(frame contains coulour:%d)", frmId, d2);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void Frame::SetPixel(uint8_t colourbit, uint8_t *buf, int x, int y, uint8_t monocolour)
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    int columns = ucihw.PixelColumns();
    int rows = ucihw.PixelRows();
    int offset = y * columns + x;
    if (colourbit == 1)
    {
        *(buf + offset / 8) |= 1 << (offset & 0x07);
    }
    else if (colourbit == 4)
    {
#ifdef HALF_BYTE
        monocolour = ucihw.GetColourXbit(monocolour);
#endif
        *(buf + offset / 2) |= (offset & 1) ? (monocolour * 0x10) : monocolour;
    }
}

int Frame::ToBit(uint8_t colourbit, uint8_t *buf)
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    int frmLen = (colourbit == 4) ? ucihw.Gfx4CoreLen() : ucihw.Gfx24CoreLen();
    memset(buf, 0, frmLen);
    if (colour < (uint8_t)FRMCOLOUR::MonoFinished)
    { // 1-bit frame
        if (colourbit == 4)
        { // mono to 4-bit/half-byte
#ifdef HALF_BYTE
            auto mappedcolour = ucihw.GetColourXbit(colour);
#else
            auto mappedcolour = ucihw.GetMappedColour(colour);
#endif
            uint8_t mappedcolourH = mappedcolour * 0x10;
            auto offset = frmOffset;
            for (int i = 0; i < frmBytes; i++, offset++)
            {
                auto data = stFrm.rawData.at(offset);
                for (int j = 0; j < 4; j++)
                {
                    *buf++ = ((data & 0x02) ? mappedcolourH : 0) + ((data & 0x01) ? mappedcolour : 0);
                    data <<= 2;
                }
            }
        }
        else // TODO 24-bit
        {
        }
    }
    else if (colour == (uint8_t)FRMCOLOUR::Multi_4bit)
    { // Multi_4bit -> 4-bit/half-byte
        if (colourbit == 4)
        { // to 4-bit
#ifdef HALF_BYTE
            for (int i = 0; i < frmBytes; i++)
            {
                auto data = *p++;
                *buf++ = ucihw.GetColourXbit((data & 0xF0) >> 4) * 0x10 + ucihw.GetColourXbit(data & 0x0F);
            }
#else
            memcpy(buf, stFrm.rawData.data() + frmOffset, frmLen);
#endif
            // TODO : 24-bit
        }
        else // TODO : to 24-bit
        {
        }
    }
    else
    { // 24-bit should not be here
    }
    return frmLen;
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
        Ldebug("Frame[%d] Error:len=%d", frmId, len);
        appErr = APP::ERROR::FrameTooLarge;
        return 1;
    }
    else if (len < (frmOffset + 2 + 1))
    {
        Ldebug("Frame[%d] Error:len=%d", frmId, len);
        appErr = APP::ERROR::FrameTooSmall;
        return 1;
    }
    return 0;
}

int FrmTxt::CheckSub(uint8_t *frm, int len)
{
    if (CheckLength(len))
    {
        return 1;
    }
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    if (micode != static_cast<uint8_t>(MI::CODE::SignSetTextFrame))
    {
        appErr = APP::ERROR::UnknownMi;
        return 1;
    }
    else if (Check::Text(frm + frmOffset, frmBytes) != 0)
    {
        Ldebug("Frame[%d] Error:Non-ASC in TextFrame", frmId);
        appErr = APP::ERROR::TextNonASC;
        return 1;
    }
    else if (!ucihw.IsFont(font))
    {
        Ldebug("Frame[%d] Error:font=%d", frmId, font);
        appErr = APP::ERROR::FontNotSupported;
        return 1;
    }
    auto pFont = ucihw.Fonts(font);
    int columns = (ucihw.PixelColumns() + pFont->CharSpacing()) / pFont->CharWidthWS();
    int rows = (ucihw.PixelRows() + pFont->LineSpacing()) / pFont->CharHeightWS();
    char *p = (char *)(frm + frmOffset);
    int lines = 0;
    int chars = 0;
    for (auto pe = p + frmBytes; p < pe; p++)
    {
        if (*p != ' ')
        {
            chars++;
            if (chars == columns)
            {
                lines++;
                chars = 0;
            }
        }
        else
        {
            if (chars > 0)
            {
                lines++;
                chars = 0;
            }
        }
    }
    if (chars > 0)
    {
        lines++;
    }
    if (lines > rows)
    {
        Ldebug("Frame[%d] Error:[%d*%d] for font[%d] but frame size is [%d] lines",
               frmId, columns, rows, font, lines);
        appErr = APP::ERROR::FrameTooLarge;
        return 1;
    }
    return 0;
}

int FrmTxt::CheckColour()
{
    if (DbHelper::Instance().GetUciHardware().IsTxtFrmColourValid(colour))
    {
        return 0;
    }
    Ldebug("Frame[%d] Error:colour=%d", frmId, colour);
    return -1;
}

std::string FrmTxt::ToString()
{
    char buf[PRINT_BUF_SIZE];
    snprintf(buf, PRINT_BUF_SIZE - 1, "MI=0x%02X(Txt), Id=%d, Rev=%d, Font=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
             micode, frmId, frmRev, font, colour, conspicuity, frmBytes, crc);
    std::string s(buf);
    return s;
}

int FrmTxt::ToBit(uint8_t colourbit, uint8_t *buf)
{
    if (colourbit == 0)
    {
        colourbit = 1;
    }
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    int frmLen = (colourbit == 1) ? ucihw.Gfx1CoreLen() : ((colourbit == 4) ? ucihw.Gfx4CoreLen() : ucihw.Gfx24CoreLen());
    memset(buf, 0, frmLen);
    auto vtext = ToStringVector();
    auto pFont = ucihw.Fonts(font);
    uint8_t monocolour = ucihw.GetMappedColour(colour);
    int start_y = (ucihw.PixelRows() - (pFont->CharHeightWS() * vtext.size() - pFont->LineSpacing())) / 2;
    for (int i = 0; i < vtext.size(); i++)
    {
        int width = pFont->GetWidth(vtext.at(i).c_str());
        int start_x = (ucihw.PixelColumns() - width) / 2;
        StrToBitmap(colourbit, buf, start_x, start_y, monocolour, vtext.at(i).c_str(), pFont);
        start_y += pFont->CharHeightWS();
    }
    // finish
    return frmLen;
}

void FrmTxt::StrToBitmap(uint8_t colourbit, uint8_t *buf, int x, int y, uint8_t monocolour, const char *str, Font *pfont)
{
    while (*str != '\0')
    {
        CharToBitmap(colourbit, buf, x, y, monocolour, *str, pfont);
        x += pfont->GetWidth(*str) + pfont->CharSpacing();
        str++;
    }
}

void FrmTxt::CharToBitmap(uint8_t colourbit, uint8_t *buf, int x, int y, uint8_t monocolour, char c, Font *pfont)
{
    int height = pfont->RowsPerCell();
    int width = pfont->GetWidth(c);
    int bytes = pfont->BytesPerCellRow();
    uint8_t *cell = pfont->GetCell(c);
    if ((*cell & 0x80) != 0)
    {
        y += pfont->Descender();
    }
    cell++; // point to pixel data
    for (int i = 0; i < height; i++)
    {
        uint32_t line = 0;
        for (int j = 0; j < 4; j++)
        {
            line <<= 8;
            line += (j < bytes) ? *cell++ : 0;
        }
        for (int j = 0; j < width; j++)
        {
            if (line & 0x80000000)
            {
                SetPixel(colourbit, buf, x + j, y + i, monocolour);
            }
            line <<= 1;
        }
    }
}

vector<string> FrmTxt::ToStringVector()
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    auto pFont = ucihw.Fonts(font);
    auto char_space = pFont->CharSpacing();
    auto line_space = pFont->LineSpacing();
    int columns = (ucihw.PixelColumns() + pFont->CharSpacing()) / pFont->CharWidthWS();
    int rows = (ucihw.PixelRows() + pFont->LineSpacing()) / pFont->CharHeightWS();
    std::vector<std::vector<char>> texts(rows, std::vector<char>(columns + 1, 0));
    auto offset = frmOffset;
    int rx = 0;
    int cx = 0;
    for (int i = 0; i < frmBytes; i++, offset++)
    {
        auto v = stFrm.rawData.at(offset);
        if (v != ' ')
        {
            texts.at(rx).at(cx) = v;
            if (++cx == columns)
            {
                if (++rx == rows)
                {
                    break;
                }
                cx = 0;
            }
        }
        else
        {
            if (cx > 0)
            {
                if (++rx == rows)
                {
                    break;
                }
                cx = 0;
            }
        }
    }
    vector<string> v;
    for (int i = 0; i < texts.size(); i++)
    {
        if (texts.at(i).at(0) != 0)
        {
            v.emplace_back(texts.at(i).data());
        }
    }
    return v;
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
    return CheckLength(len) || CheckMultiColour(frm, len);
}

int FrmGfx::CheckColour()
{
    if (DbHelper::Instance().GetUciHardware().IsGfxFrmColourValid(colour))
    {
        return 0;
    }
    Ldebug("Frame[%d] Error:colour=%d", frmId, colour);
    return -1;
}

std::string FrmGfx::ToString()
{
    char buf[PRINT_BUF_SIZE];
    snprintf(buf, PRINT_BUF_SIZE - 1, "MI=0x%02X(Gfx), Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
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
    return CheckLength(len) || CheckMultiColour(frm, len);
}

int FrmHrg::CheckColour()
{
    if (DbHelper::Instance().GetUciHardware().IsHrgFrmColourValid(colour))
    {
        return 0;
    }
    Ldebug("Frame[%d] Error:colour=%d", frmId, colour);
    return -1;
}

std::string FrmHrg::ToString()
{
    char buf[PRINT_BUF_SIZE];
    snprintf(buf, PRINT_BUF_SIZE - 1, "MI=0x%02X(Hrg), Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
             micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmBytes, crc);
    std::string s(buf);
    return s;
}
