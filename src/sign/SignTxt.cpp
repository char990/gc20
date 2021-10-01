#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>

uint8_t *SignTxt::GetExtStatus(uint8_t *buf)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=prod.ExtStsRplSignType();
    *p++=prod.CharRows(0);
    *p++=prod.CharColumns(0);
    *p++=signErr;
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++;
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}
