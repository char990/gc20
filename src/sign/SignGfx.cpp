#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignGfx.h>

SignGfx::SignGfx(uint8_t signId)
:SignTxt(signId)
{

}

SignGfx::~SignGfx()
{

}

uint8_t *SignGfx::GetExtStatus(uint8_t *pbuf)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    uint8_t *p=pbuf;
    *p++=signId;
    uint8_t signtype=prod.ExtStsRplSignType();
    *p++=signtype;
    switch(signtype)
    {
        case 1:
        *p++=prod.PixelRows();
        *p++=prod.PixelColumns();
        *p++=signErr;
        *p++=dimMode;
        *p++=dimLevel;
        break;
        case 2:
        *p++=prod.CharRows(0);
        *p++=prod.CharColumns(0);
        *p++=signErr;
        *p++=dimMode;
        *p++=dimLevel;
        *p++=prod.PixelRows()/0x100;
        *p++=prod.PixelRows()&0xff;
        *p++=prod.PixelColumns()/0x100;
        *p++=prod.PixelColumns()&0xff;
        {
            Font * font = prod.Fonts(0);
            *p++=font->ColumnsPerCell();
            *p++=font->RowsPerCell();
            *p++=font->CharSpacing();
            *p++=font->LineSpacing();
            *p++=strlen(prod.ColourLeds());
            *p++=prod.ColourBits();
        }
        break;
    }
    *p++=0;
    return p;
}
