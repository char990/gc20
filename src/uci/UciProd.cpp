#include <string>
#include <uci/UciProd.h>
#include <tsisp003/TsiSp003Const.h>

UciProd::UciProd()
{
    uciOpt.path = const_cast<char *>("/etc/config");
    uciOpt.package = const_cast<char *>("goblin");
}

void UciProd::LoadConfig()
{

    Dump();
}

void UciProd::Dump()
{


}

uint8_t UciProd::TsiSp003Ver()
{
    return 0x50;
}


char * UciProd::MfcCode()
{
    return  const_cast<char *>("GC20123456");
}

int UciProd::ColourBits()
{
    return 1;
    return 4;
    return 24;
}

int UciProd::MaxTextFrmLen()
{
    return 255;
}

int UciProd::MinGfxFrmLen() 
{
    return (Pixels()+7)/8;
}

int UciProd::MaxGfxFrmLen() 
{
    switch(ColourBits())
    {
        case 1:
            return (Pixels()+7)/8;
        case 4:
            return (Pixels()+2)/2;
        default:
            return 0;
    }
}

int UciProd::MinHrgFrmLen() 
{
    return (Pixels()+7)/8;
}

int UciProd::MaxHrgFrmLen() 
{
    switch(ColourBits())
    {
        case 1:
            return (Pixels()+7)/8;
        case 4:
            return (Pixels()+2)/2;
        default:
            return (Pixels()*3);
    }
}

int UciProd::CharRows() 
{
    return 3;
}

int UciProd::CharColumns() 
{
    return 18;
}

int UciProd::PixelRows() 
{
    return 64;
}

int UciProd::PixelColumns() 
{
    return 288;
}

int UciProd::Pixels() 
{
    return PixelRows()*PixelColumns();
}

