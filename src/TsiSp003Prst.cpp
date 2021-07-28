#include "TsiSp003Prst.h"
#include "Utilities.h"


int TsiSp003Prst::Rx(uint8_t * data, int len)
{
    if(len&1)
    {
        return -1;
    }
    len = len/2;
    if(len>BUF_SIZE)
    {
        return -1;
    }
    uint8_t * src=data;
    uint8_t * dst=buf;
    for(int i=0;i<len;i++)
    {
        int x = Util::Cnvt::ParseAscToHex(src);
        if(x==-1)
        {
            return -1;
        }
        src+=2;
        *dst++=x;
    }
    return upperLayer->Rx(buf, len);
}

int TsiSp003Prst::Tx(uint8_t * data, int len)
{
    if(len<=0 || len>(BUF_SIZE/2))
    {
        return -1;
    }
    uint8_t * src=data;
    uint8_t * dst=buf;
    for(int i=0;i<len;i++)
    {
        Util::Cnvt::ParseHexToAsc(*src, dst);
        src++;
        dst+=2;
    }
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

