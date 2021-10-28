#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignGfx.h>
#include <module/Utils.h>

using namespace Utils;
uint8_t *SignGfx::GetExtStatus(uint8_t *buf)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(prod.ExtStsRplSignType());
    *p++=prod.PixelRows();
    *p++=prod.PixelColumns();
    *p++=static_cast<uint8_t>(signErr.GetErrorCode());
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++;
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}

uint8_t *SignGfx::GetConfig(uint8_t *buf)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(prod.ConfigRplSignType());
    p=Cnvt::PutU16(prod.PixelColumns(),p);
    p=Cnvt::PutU16(prod.PixelRows(),p);
    return p;
}

