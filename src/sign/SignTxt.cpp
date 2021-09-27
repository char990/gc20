#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>

uint8_t *SignTxt::GetExtStatus(uint8_t *p)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    *p++=signId;
    uint8_t signtype=prod.ExtStsRplSignType();
    *p++=signtype;
    *p++=prod.CharRows(0);
    *p++=prod.CharColumns(0);
    *p++=signErr;
    *p++=DimmingMode();
    *p++=DimmingValue();
    *p++=0;
    return p;
}