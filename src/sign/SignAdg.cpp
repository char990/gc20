#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignAdg.h>
#include <module/Utils.h>

using namespace Utils;

uint8_t *SignAdg::GetExtStatus(uint8_t *buf)
{
    auto & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(prod.ExtStsRplSignType());
    *p++=prod.CharRows(0);
    *p++=prod.CharColumns(0);
    *p++=static_cast<uint8_t>(signErr.GetErrorCode());
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++; // [7]: led status bytes
    p=Cnvt::PutU16(prod.PixelColumns(),p);
    p=Cnvt::PutU16(prod.PixelRows(),p);
    Font * font = prod.Fonts(0);
    *p++=font->ColumnsPerCell();
    *p++=font->RowsPerCell();
    *p++=font->CharSpacing();
    *p++=font->LineSpacing();
    *p++=strlen(prod.ColourLeds());
    *p++=prod.ColourBits();
    p=Cnvt::PutU16(faultLedCnt,p);
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}

uint8_t *SignAdg::GetConfig(uint8_t *buf)
{
    auto & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(prod.ConfigRplSignType());
    p=Cnvt::PutU16(prod.PixelColumns(),p);
    p=Cnvt::PutU16(prod.PixelRows(),p);
    return p;
}

