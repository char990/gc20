#include <tsisp003/TsiSp003Prst.h>
#include <module/Utils.h>


int TsiSp003Prst::Rx(uint8_t * data, int len)
{
    if(len>(MAX_APP_PACKET_SIZE*2))
    {
        return -1;
    }
    int x = Utils::Cnvt::ParseToU8((char *)data, buf, len);
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
    Utils::Cnvt::ParseToAsc(data, (char *)buf, len);
    return lowerLayer->Tx(buf, len*2);
}

