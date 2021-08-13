#include <stdexcept>
#include <cstring>

#include <tsisp003/Frame.h>
#include <module/Utils.h>
#include <tsisp003/TsiSp003Const.h>


using namespace Utils;

Frame::Frame()
:frmData(nullptr),micode(0)
{
}

void Frame::CnvtFrm(char * frm, int len, int max)
{
    if((len&1)==1 || len>(max)*2)
    {
        appErr = APP::ERROR::LengthError;
        return;
    }
    len=len/2;
    frmData = new uint8_t [len];
    if(Cnvt::ParseToU8(frm, frmData, len*2)!=0)
    {
        appErr = APP::ERROR::LengthError;
    }
    else
    {
        appErr = APP::ERROR::AppNoError;
    }
}

int Frame::CheckConspicuity(uint8_t v)
{
    return 0;
}


/************************************************************/

FrmTxt::FrmTxt(char * frm, int len)
{
    CnvtFrm(frm, len, MAX_TXTFRM_SIZE);
    if(appErr==APP::ERROR::AppNoError)
    {
        Frame(len/2);
    }
}

FrmTxt::FrmTxt(uint8_t * frm, int len)
{
    if(len<(9+1) || len>MAX_TXTFRM_SIZE)
    {
        appErr = APP::ERROR::LengthError;
    }
    else
    {
        frmData = new uint8_t [len];
        memcpy(frmData, frm, len);
        Frame(len/2);
    }
}

void FrmTxt::Frame(int len)
{
    if(frmData[0]!=MI::CODE::SignSetTextFrame)
    {
        appErr = APP::ERROR::UnknownMi;
    }
    else if(len!=(9+frmData[6]) || len>(255+9))
    {
        appErr = APP::ERROR::LengthError;
    }
    else if (Check::Text(frmData+7, len-9)!=0)
    {
        appErr = APP::ERROR::TextNonASC;
    }
    else if(CheckFont(frmData[3])!=0) // font
    {
        appErr = APP::ERROR::FontNotSupported;
    }
    else if(CheckColour(frmData[4])!=0) // colour
    {
        appErr = APP::ERROR::ColourNotSupported;
    }
    else if(CheckConspicuity(frmData[5])!=0) // Conspicuity
    {
        appErr = APP::ERROR::ConspicuityNotSupported;
    }
    else
    {
        uint16_t c = Crc::Crc16_1021(frmData, len-2);
        if( (c/0x100) == frmData[len-2] || (c&0xFF) == frmData[len-1] )
        {
            crc=c; 
            appErr = APP::ERROR::AppNoError;
            micode=frmData[0];
            frmId=frmData[1];
            frmRev=frmData[2];
            font=frmData[3];
            colour=frmData[4];
            conspicuity=frmData[5];
            frmlen=frmData[6];
            return;
        }
        appErr = APP::ERROR::DataChksumError;
    }
}

FrmTxt::~FrmTxt()
{
    if(frmData !=nullptr)
    {
        delete [] frmData;
    }
}

int FrmTxt::CheckFont(uint8_t font)
{
    return 0;
}

int FrmTxt::CheckColour(uint8_t font)
{
    return 0;
}

std::string FrmTxt::ToString()
{
    char buf[1024];
    snprintf(buf,1023,"TextFrame:(appErr=%s) MI=0x%02X, Id=%d, Rev=%d, Font=%d, Colour=%d, Consp=%d, Len=%d, Crc=0x%04X",
        appErr, micode, frmId, frmRev, font, colour, conspicuity, frmlen, crc);
    std::string s(buf);
    return s;
}

FrmGfx::FrmGfx(char * frm, int len)
{

}

FrmGfx::FrmGfx(uint8_t * frm, int len)
{

}

FrmGfx::~FrmGfx()
{
    if(frmData !=nullptr)
    {
        delete [] frmData;
    }
}

std::string FrmGfx::ToString()
{
    std::string s;
    return s;
}

FrmHrg::FrmHrg(char * frm, int len)
{

}

FrmHrg::FrmHrg(uint8_t * frm, int len)
{

}

FrmHrg::~FrmHrg()
{
    if(frmData !=nullptr)
    {
        delete [] frmData;
    }
}

std::string FrmHrg::ToString()
{
    std::string s;
    return s;
}
