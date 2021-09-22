#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignAdg.h>

uint8_t *SignAdg::GetExtStatus(uint8_t *p)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    uint8_t *p=pbuf;
    *p++=signId;
    *p++=prod.ExtStsRplSignType();
    *p++=prod.CharRows(0);
    *p++=prod.CharColumns(0);
    *p++=signErr;
    *p++=dimMode;
    *p++=dimLevel;
    *p++=12;
    *p++=prod.PixelColumns()/0x100;
    *p++=prod.PixelColumns()&0xff;
    *p++=prod.PixelRows()/0x100;
    *p++=prod.PixelRows()&0xff;
    {
        Font * font = prod.Fonts(0);
        *p++=font->ColumnsPerCell();
        *p++=font->RowsPerCell();
        *p++=font->CharSpacing();
        *p++=font->LineSpacing();
        *p++=strlen(prod.ColourLeds());
        *p++=prod.ColourBits();
        *p++=0;
        *p++=0;
    }
    return p;
}
