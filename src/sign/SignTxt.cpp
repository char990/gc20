#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>

SignTxt::SignTxt(uint8_t signId)
:Sign(signId)
{

}

SignTxt::~SignTxt()
{

}

uint8_t *SignTxt::GetExtStatus(uint8_t *pbuf)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    uint8_t *p=pbuf;
    *p++=signId;
    uint8_t signtype=prod.ExtStsRplSignType();
    *p++=signtype;
    *p++=prod.CharRows(0);
    *p++=prod.CharColumns(0);
    *p++=signErr;
    *p++=dimMode;
    *p++=dimLevel;
    *p++=0;
    return p;
}

