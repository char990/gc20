#include <cstring>
#include <sign/Vms.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignAdg.h>

/*
    uint8_t signId, power;
    uint8_t dimmingSet, dimmingV;
    uint8_t deviceSet, deviceV;

    DispStatus dsBak;
    DispStatus dsCurrent;
    DispStatus dsNext;
    DispStatus dsExt;
        uint8_t currentPln, currentMsg, currentFrm;
*/


Vms::Vms(uint8_t sid)
:UnitedSign(id)
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
        sign = new SignAdg(sid);
    }
}

Vms::~Vms()
{
    delete [] slaves;
    delete sign;
}

void Vms::Reset()
{
    dsBak.dispType = DISP_STATUS::TYPE::N_A;
    dsCurrent.dispType = DISP_STATUS::TYPE::N_A;
    dsNext.dispType = DISP_STATUS::TYPE::N_A;
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
