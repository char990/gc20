#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignAdg.h>
#include <module/Utils.h>

using namespace Utils;

uint8_t *SignAdg::GetExtStatus(uint8_t *buf)
{
    auto & ucihw = DbHelper::Instance().GetUciHardware();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(ucihw.ExtStsRplSignType());
    *p++=ucihw.CharRows(0);
    *p++=ucihw.CharColumns(0);
    *p++=static_cast<uint8_t>(signErr.GetErrorCode());
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++; // [7]: led status bytes
    p=Cnvt::PutU16(ucihw.PixelColumns(),p);
    p=Cnvt::PutU16(ucihw.PixelRows(),p);
    Font * font = ucihw.Fonts(0);
    *p++=font->ColumnsPerCell();
    *p++=font->RowsPerCell();
    *p++=font->CharSpacing();
    *p++=font->LineSpacing();
    *p++=strlen(ucihw.ColourLeds());
    *p++=ucihw.ColourBits();
    p=Cnvt::PutU16(faultLedCnt,p);
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}

uint8_t *SignAdg::GetConfig(uint8_t *buf)
{
    auto & ucihw = DbHelper::Instance().GetUciHardware();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(ucihw.ConfigRplSignType());
    p=Cnvt::PutU16(ucihw.PixelColumns(),p);
    p=Cnvt::PutU16(ucihw.PixelRows(),p);
    return p;
}

