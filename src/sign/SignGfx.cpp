#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignGfx.h>

uint8_t *SignGfx::GetExtStatus(uint8_t *buf)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=prod.ExtStsRplSignType();
    *p++=prod.PixelRows();
    *p++=prod.PixelColumns();
    *p++=signErr;
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++;
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}
