#include <layer/SLV_LayerManager.h>
#include <layer/LayerSlv.h>
#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <sign/Slave.h>

SLV_LayerManager::SLV_LayerManager(std::string name_, int groupId, IUpperLayer * upperLayer)
{
    maxPktSize = 1 + 9 + DbHelper::Instance().GetUciHardware().MaxCoreLen() + 2;
    prstLayer = new LayerPrst(maxPktSize);
    dlLayer = new LayerSlv(name_, groupId, (maxPktSize+2)*2+2);  // 0x02 + ([packet] + 2-byte CRC)*2 + 0x03
    appLayer = upperLayer;
    // lowerLayer<->dlLayer<->prstLayer<->appLayer
    // dlLayer layer, need lower&upper layer
    // dlLayer->LowerLayer(lowerLayer); set in this->LowerLayer(lowerLayer)
    dlLayer->UpperLayer(prstLayer);
    // presentation layer, need lower&upper layer
    prstLayer->LowerLayer(dlLayer);
    prstLayer->UpperLayer(appLayer);
    // app layer, only need lower layer
    appLayer->LowerLayer(prstLayer);
}

SLV_LayerManager::~SLV_LayerManager()
{
    if(prstLayer)
    {
        delete prstLayer;
    }
    if(dlLayer)
    {
        delete dlLayer;
    }
}

int SLV_LayerManager::Rx(uint8_t * data, int len)
{
    return dlLayer->Rx(data,len);
}

void SLV_LayerManager::ClrRx()
{
    return dlLayer->ClrRx();
}
