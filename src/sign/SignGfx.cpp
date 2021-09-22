#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignGfx.h>

uint8_t *SignGfx::GetExtStatus(uint8_t *pbuf)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    uint8_t *p=pbuf;
    *p++=signId;
    *p++=prod.ExtStsRplSignType();
    *p++=prod.PixelRows();
    *p++=prod.PixelColumns();
    *p++=signErr;
    *p++=dimMode;
    *p++=dimLevel;
    *p++=0;
    return p;
}
