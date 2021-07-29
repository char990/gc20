#include "TsiSp003Prst.h"
#include "Utilities.h"


int TsiSp003Prst::Rx(uint8_t * data, int len)
{
    if(len>(BUF_SIZE*2))
    {
        return -1;
    }
    int x = Util::Cnvt::ParseAscToHex(data, buf, len);
    if(x==-1)
    {
        return -1;
    }
    return upperLayer->Rx(buf, len/2);
}

int TsiSp003Prst::Tx(uint8_t * data, int len)
{
    if(len<=0 || len>(BUF_SIZE/2))
    {
        return -1;
    }
    Util::Cnvt::ParseHexToAsc(data, buf, len);
    return lowerLayer->Rx(buf, len*2);
}

void TsiSp003Prst::PeriodicRun()
{
    upperLayer->PeriodicRun();
}

void TsiSp003Prst::Clean()
{
    upperLayer->Clean();
}

void TsiSp003Prst::Release()
{
    lowerLayer->Release();
}

