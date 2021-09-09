#include <stdexcept>
#include <cstring>

#include <tsisp003/Frame.h>
#include <module/Utils.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>


using namespace Utils;

Frame::Frame()
:frmData(nullptr),micode(0)
{
}

void Frame::CnvtFrm(char * frm, int len, int min, int max)
{
    if((len&1)==1)
    {
        appErr = APP::ERROR::LengthError;
    }
    else if(len<min*2)
    {
        appErr = APP::ERROR::FrameTooSmall;
    }
    else if(len>max*2)
    {
        appErr = APP::ERROR::FrameTooLarge;
    }
    else
    {
        len=len/2;
        frmData = new uint8_t [len];
        if(Cnvt::ParseToU8(frm, frmData, len*2)!=0)
        {
            appErr = APP::ERROR::SyntaxError;
        }
        else
        {
            appErr = APP::ERROR::AppNoError;
        }
    }
}

int Frame::CheckConspicuity()
{
    return 0;
}


/************************************************************/

FrmTxt::FrmTxt(char * frm, int len)
{
    frmOffset=TXTFRM_HEADER_SIZE;
    CnvtFrm(frm, len, (frmOffset+2+1), (frmOffset+2+255));
    if(appErr==APP::ERROR::AppNoError)
    {
        MakeFrame(len/2);
    }
}

FrmTxt::FrmTxt(uint8_t * frm, int len)
{
    frmOffset=TXTFRM_HEADER_SIZE;
    if(len<(frmOffset+2+1))
    {
        appErr = APP::ERROR::FrameTooSmall;
    }
    else if(len>255 || 0/* if text could fit in the sign: X chars * Y chars*/)
    {
        appErr = APP::ERROR::FrameTooLarge;
    }
    else
    {
        frmData = new uint8_t [len];
        memcpy(frmData, frm, len);
        MakeFrame(len);
    }
}

void FrmTxt::MakeFrame(int len)
{
    micode=frmData[0];
    frmId=frmData[1];
    frmRev=frmData[2];
    font=frmData[3];
    colour=frmData[4];
    conspicuity=frmData[5];
    frmlen=frmData[6];
    crc = Crc::Crc16_1021(frmData, len-2);
    if(micode!=MI::CODE::SignSetTextFrame)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if(frmId==0)
    {
        appErr = APP::ERROR::SyntaxError;
    }
    else if(CheckColour()!=0) // colour
    {
        appErr = APP::ERROR::ColourNotSupported;
    }
    else if(CheckConspicuity()!=0) // Conspicuity
    {
        appErr = APP::ERROR::ConspicuityNotSupported;
    }
    else if (Check::Text(frmData+frmOffset, len-(frmOffset+2))!=0)
    {
        appErr = APP::ERROR::TextNonASC;
    }
    else if(CheckFont()!=0) // font
    {
        appErr = APP::ERROR::FontNotSupported;
    }
    else
    {
        CheckLength(len);
        if(appErr == APP::ERROR::AppNoError)
        {
            if(crc != (frmData[len-2]*0x100+frmData[len-1]) )
            {
                appErr = APP::ERROR::DataChksumError;
            }
        }
    }
}

FrmTxt::~FrmTxt()
{
    if(frmData !=nullptr)
    {
        delete [] frmData;
    }
}

void FrmTxt::CheckLength(int len)
{
    if(len!=(frmOffset+2+frmlen))
    {
        appErr = APP::ERROR::LengthError;
    }
    else if(len>(255+frmOffset+2))
    {
        appErr = APP::ERROR::FrameTooLarge;
    }
    else if(len<(frmOffset+2+1))
    {
        appErr = APP::ERROR::FrameTooSmall;
    }
}

int FrmTxt::CheckFont()
{
    return 0;
}

int FrmTxt::CheckColour()
{
    return 0;
}

std::string FrmTxt::ToString()
{
    char buf[1024];
    snprintf(buf,1023,"TextFrame:(appErr=%d) MI=0x%02X, Id=%d, Rev=%d, Font=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
        appErr, micode, frmId, frmRev, font, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}

FrmGfx::FrmGfx(char * frm, int len)
{
    frmOffset=GFXFRM_HEADER_SIZE;
    CnvtFrm(frm, len,
        frmOffset+2+DbHelper::Instance().uciProd.MinGfxFrmLen(),
        frmOffset+2+DbHelper::Instance().uciProd.MaxGfxFrmLen());
    if(appErr==APP::ERROR::AppNoError)
    {
        MakeFrame(len/2);
    }
}

FrmGfx::FrmGfx(uint8_t * frm, int len)
{
    frmOffset=GFXFRM_HEADER_SIZE;
    if( len < frmOffset+2+DbHelper::Instance().uciProd.MinGfxFrmLen() ||
        len > frmOffset+2+DbHelper::Instance().uciProd.MaxGfxFrmLen())
    {
        appErr = APP::ERROR::LengthError;
    }
    else
    {
        appErr = APP::ERROR::AppNoError;
        frmData = new uint8_t [len];
        memcpy(frmData, frm, len);
        MakeFrame(len);
    }
}

FrmGfx::~FrmGfx()
{
    if(frmData !=nullptr)
    {
        delete [] frmData;
    }
}

void FrmGfx::MakeFrame(int len)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    micode=frmData[0];
    frmId=frmData[1];
    frmRev=frmData[2];
    pixelRows=frmData[3];
    pixelColumns=frmData[4];
    colour=frmData[5];
    conspicuity=frmData[6];
    frmlen=frmData[7]*0x100+frmData[8];
    crc = Crc::Crc16_1021(frmData, len-2);
    if(micode!=MI::CODE::SignSetGraphicsFrame)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if(frmId==0)
    {
        appErr = APP::ERROR::SyntaxError;
    }
    else if(pixelRows!=prod.PixelRows() || pixelColumns!=prod.PixelColumns()) // rows & columns
    {
        appErr = APP::ERROR::SizeMismatch;
    }
    else if(CheckColour()!=0) // colour
    {
        appErr = APP::ERROR::ColourNotSupported;
    }
    else if(CheckConspicuity()!=0) // Conspicuity
    {
        appErr = APP::ERROR::ConspicuityNotSupported;
    }
    else
    {
        CheckLength(len);
        if(appErr == APP::ERROR::AppNoError)
        {
            if(crc != (frmData[len-2]*0x100+frmData[len-1]) )
            {
                appErr = APP::ERROR::DataChksumError;
            }
        }
    }
}

void FrmGfx::CheckLength(int len)
{
    int pixels = DbHelper::Instance().uciProd.Pixels();
    if(len!=(frmOffset+2+frmlen))
    {
        appErr = APP::ERROR::LengthError;
    }
    else
    {
        int x=0;
        if(colour<FRM::COLOUR::MultipleColours)
        {
            x = (pixels+7)/8;
        }
        else if(colour == FRM::COLOUR::MultipleColours)
        {
            x = (pixels+1)/2;
        }
        if(x!=0)
        {
            if(frmlen>x)
            {
                appErr = APP::ERROR::FrameTooLarge;
            }
            else if(frmlen<x)
            {
                appErr = APP::ERROR::FrameTooSmall;
            }
        }
        else
        {
            appErr = APP::ERROR::ColourNotSupported;
        }
    }
}

int FrmGfx::CheckColour()
{
    return 0;
}


std::string FrmGfx::ToString()
{
    char buf[1024];
    snprintf(buf,1023,"GfxFrame:(appErr=%d) MI=0x%02X, Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
        appErr, micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}

/****************************** FrmHrg *******************************/
FrmHrg::FrmHrg(char * frm, int len)
{
    frmOffset=HRGFRM_HEADER_SIZE;
    CnvtFrm(frm, len,
        frmOffset+2+DbHelper::Instance().uciProd.MinGfxFrmLen(),
        frmOffset+2+DbHelper::Instance().uciProd.MaxGfxFrmLen());
    if(appErr==APP::ERROR::AppNoError)
    {
        MakeFrame(len/2);
    }
}

FrmHrg::FrmHrg(uint8_t * frm, int len)
{
    frmOffset=HRGFRM_HEADER_SIZE;
    if( len < frmOffset+2+DbHelper::Instance().uciProd.MinGfxFrmLen() ||
        len > frmOffset+2+DbHelper::Instance().uciProd.MaxGfxFrmLen())
    {
        appErr = APP::ERROR::LengthError;
    }
    else
    {
        appErr=APP::ERROR::AppNoError;
        frmData = new uint8_t [len];
        memcpy(frmData, frm, len);
        MakeFrame(len);
    }
}

FrmHrg::~FrmHrg()
{
    if(frmData !=nullptr)
    {
        delete [] frmData;
    }
}


void FrmHrg::MakeFrame(int len)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    micode=frmData[0];
    frmId=frmData[1];
    frmRev=frmData[2];
    pixelRows=frmData[3]*0x100+frmData[4];
    pixelColumns=frmData[5]*0x100+frmData[6];
    colour=frmData[7];
    conspicuity=frmData[8];
    frmlen=0;
    for(int i=0;i<4;i++)
    {
        frmlen*=0x100;
        frmlen+=frmData[9+i];
    }
    crc = Crc::Crc16_1021(frmData, len-2);
    if(micode!=MI::CODE::SignSetHighResolutionGraphicsFrame)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if(frmId==0)
    {
        appErr = APP::ERROR::SyntaxError;
    }
    else if(pixelRows!=prod.PixelRows() || pixelColumns!=prod.PixelColumns()) // rows & columns
    {
        appErr = APP::ERROR::SizeMismatch;
    }
    else if(CheckColour()!=0) // colour
    {
        appErr = APP::ERROR::ColourNotSupported;
    }
    else if(CheckConspicuity()!=0) // Conspicuity
    {
        appErr = APP::ERROR::ConspicuityNotSupported;
    }
    else
    {
        CheckLength(len);
        if(appErr == APP::ERROR::AppNoError)
        {
            if(crc != (frmData[len-2]*0x100+frmData[len-1]) )
            {
                appErr = APP::ERROR::DataChksumError;
            }
        }
    }
}

void FrmHrg::CheckLength(int len)
{
    if(len!=(frmOffset+2+frmlen))
    {
        appErr = APP::ERROR::LengthError;
    }
    else
    {
        int pixels = DbHelper::Instance().uciProd.Pixels();
        int x=0;
        if(colour<FRM::COLOUR::MultipleColours)
        {
            x = (pixels+7)/8;
        }
        else if(colour == FRM::COLOUR::MultipleColours)
        {
            x = (pixels+1)/2;
        }
        else if(colour == FRM::COLOUR::RGB24)
        {
            x = pixels*3;
        }
        if(x!=0)
        {
            if(frmlen>x)
            {
                appErr = APP::ERROR::FrameTooLarge;
            }
            else if(frmlen<x)
            {
                appErr = APP::ERROR::FrameTooSmall;
            }
        }
        else
        {
            appErr = APP::ERROR::ColourNotSupported;
        }
    }
}

int FrmHrg::CheckColour()
{
    return 0;
}

std::string FrmHrg::ToString()
{
    char buf[1024];
    snprintf(buf,1023,"HrgFrame:(appErr=%d) MI=0x%02X, Id=%d, Rev=%d, Rows=%d, Columns=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
        appErr, micode, frmId, frmRev, pixelRows, pixelColumns, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}
