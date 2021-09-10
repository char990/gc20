#include <cstring>
#include <sign/Vms.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignHrg.h>

Vms::Vms(uint8_t sid)
{
    UciProd & prod = DbHelper::Instance().uciProd;
    Slave::numberOfTiles = prod.TileRowsPerSlave() * prod.TileColumnsPerSlave();
    Slave::numberOfColours = strlen(prod.ColourLeds());
    numberOfSlaves = prod.SlaveRowsPerSign() * prod.SlaveColumnsPerSign();
    slaves = new Slave[numberOfSlaves];
    for(int i=0;i<numberOfSlaves;i++)
    {
        slaves[i].slaveId = i+1; 
    }
    if(0)
    {
        sign = new SignTxt(sid);
    }
    else if (1)
    {
        sign = new SignGfx(sid);
    }
    else if (2)
    {
        sign = new SignHrg(sid);
    }
}

Vms::~Vms()
{
    delete [] slaves;
    delete sign;
}

uint8_t * Vms::GetStatus(uint8_t *p)
{
    return sign->GetStatus(p);
}

uint8_t * Vms::GetExtStatus(uint8_t *pbuf)
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
