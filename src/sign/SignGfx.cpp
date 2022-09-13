#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignGfx.h>
#include <module/Utils.h>

using namespace Utils;
uint8_t *SignGfx::GetExtStatus(uint8_t *buf)
{
    auto & ucihw = DbHelper::Instance().GetUciHardware();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(ucihw.ExtStsRplSignType());
    *p++=ucihw.PixelRows();
    *p++=ucihw.PixelColumns();
    *p++=static_cast<uint8_t>(signErr.GetErrorCode());
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++; // [7]: led status bytes
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}

uint8_t *SignGfx::GetConfig(uint8_t *buf)
{
    auto & ucihw = DbHelper::Instance().GetUciHardware();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(ucihw.ConfigRplSignType());
    p=Cnvt::PutU16(ucihw.PixelColumns(),p);
    p=Cnvt::PutU16(ucihw.PixelRows(),p);
    return p;
}

