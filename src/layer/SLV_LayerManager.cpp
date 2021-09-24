#include <layer/SLV_LayerManager.h>
#include <layer/LayerSlv.h>
#include <module/MyDbg.h>
#include <uci/DbHelper.h>
#include <sign/Slave.h>

SLV_LayerManager::SLV_LayerManager(std::string name_, std::string aType)
{
    if(aType.compare("SLV")==0)
    {
        UciProd &prod = DbHelper::Instance().uciProd;
        appLayer = new GroupApp();
        maxPktSize = 22 + Slave::numberOfTiles*Slave::numberOfColours;
        prstLayer = new LayerPrst(maxPktSize);
        dlLayer = new LayerSlv(name_, maxPktSize*2+2);
    }
    else
    {
        MyThrow ("Unkown adaptor type:%s", aType.c_str());
    }
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
    if(appLayer)
    {
        delete appLayer;
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

void SLV_LayerManager::Clean()
{
    return dlLayer->Clean();
}
