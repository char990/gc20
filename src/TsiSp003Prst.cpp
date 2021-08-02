#include "TsiSp003Prst.h"
#include "Utilities.h"


int TsiSp003Prst::Rx(uint8_t * data, int len)
{
    if(len>(MAX_APP_PACKET_SIZE*2))
    {
        return -1;
    }
    int x = Util::Cnvt::ParseToU8(data, buf, len);
    if(x==-1)
    {
        return -1;
    }
    return upperLayer->Rx(buf, len/2);
}

void TsiSp003Prst::Clean()
{
    upperLayer->Clean();
}

int TsiSp003Prst::Tx(uint8_t * data, int len)
{
    if(len<=0 || len>(MAX_APP_PACKET_SIZE))
    {
        return -1;
    }
    Util::Cnvt::ParseToAsc(data, buf, len);
    return lowerLayer->Tx(buf, len*2);
}

