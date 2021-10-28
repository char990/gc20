#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <module/Utils.h>

using namespace Utils;
uint8_t *SignTxt::GetExtStatus(uint8_t *buf)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(prod.ExtStsRplSignType());
    *p++=prod.CharRows(0);
    *p++=prod.CharColumns(0);
    *p++=static_cast<uint8_t>(signErr.GetErrorCode());
    *p++=DimmingMode();
    *p++=DimmingValue();
    p++;
    p=LedStatus(p);
    buf[7] = p-buf-8;
    return p;
}

uint8_t *SignTxt::GetConfig(uint8_t *buf)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    uint8_t * p = buf;
    *p++=signId;
    *p++=static_cast<uint8_t>(prod.ConfigRplSignType());
    p=Cnvt::PutU16(prod.CharRows(0),p);
    p=Cnvt::PutU16(prod.CharColumns(0),p);
    return p;
}

