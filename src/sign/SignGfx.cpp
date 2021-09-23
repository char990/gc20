#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignGfx.h>

uint8_t *SignGfx::GetExtStatus(uint8_t *p)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    *p++=signId;
    *p++=prod.ExtStsRplSignType();
    *p++=prod.PixelRows();
    *p++=prod.PixelColumns();
    *p++=signErr;
    *p++=DimmingMode();
    *p++=DimmingValue();
    *p++=0;
    return p;
}
