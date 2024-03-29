#include <module/Utils.h>
#include <module/MyDbg.h>
#include <layer/LayerPrst.h>
#include <module/Utils.h>

using namespace Utils;

LayerPrst::LayerPrst(int maxlen)
:maxlen(maxlen)
{
    if(maxlen<=0)
    {
        throw std::invalid_argument(StrFn::PrintfStr("LayerPrst::maxlen error : %d",maxlen));
    }
    buf = new uint8_t[maxlen*2];
}

LayerPrst::~LayerPrst()
{
    delete [] buf;
}

int LayerPrst::Rx(uint8_t * data, int len)
{
    if(len>maxlen*2)
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

void LayerPrst::ClrRx()
{
    upperLayer->ClrRx();
}


bool LayerPrst::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerPrst::Tx(uint8_t * data, int len)
{
    if(len<=0 || len>(maxlen))
    {
        return -1;
    }
    Utils::Cnvt::ParseToAsc(data, (char *)buf, len);
    return lowerLayer->Tx(buf, len*2);
}

void LayerPrst::ClrTx()
{
    lowerLayer->ClrTx();
}