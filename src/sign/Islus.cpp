#include <sign/Islus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignHrg.h>

Islus::Islus(uint8_t sid)
{
    sign = new SignTxt(sid);
}

Islus::~Islus()
{
    delete sign;
}

uint8_t * Islus::GetStatus(uint8_t *p)
{
    return sign->GetStatus(p);
}

uint8_t * Islus::GetExtStatus(uint8_t *pbuf)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    uint8_t *p=sign->GetExtStatus(pbuf);
    int tiles = prod.TileRowsPerSlave()*prod.TileColumnsPerSlave()*prod.SlaveRowsPerSign()*prod.SlaveColumnsPerSign();
    int bytes = (tiles+7)/8;
    pbuf[7]+=bytes;
    for(int i=0;i<bytes;i++)
    {
        *p++=0;
    }
    return p;
}
