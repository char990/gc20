#include <layer/LayerManager.h>
#include <layer/LayerNTS.h>
#include <layer/LayerWeb.h>
#include <layer/LayerDL.h>
#include <module/MyDbg.h>
LayerManager::LayerManager(std::string name_, std::string aType)
{
    if(aType.compare("NTS")==0)
    {
        appFactory = new AppFactory();
        appLayer = appFactory->GetApp();
        prstLayer = new LayerPrst(MAX_DATA_PACKET_SIZE);
        dlLayer = new LayerDL(name_, MAX_DATA_PACKET_SIZE);
        LayerNTS *layerNTS = new LayerNTS(name_);
        midLayer = layerNTS;
        //ISession *s = layerNTS;
        appLayer->SetSession((ISession *)layerNTS);
    }
    else if(aType.compare("WEB")==0)
    {
        appFactory = new AppFactory();
        appLayer = appFactory->GetApp();
        prstLayer = new LayerPrst(MAX_DATA_PACKET_SIZE);
        dlLayer = new LayerDL(name_, MAX_DATA_PACKET_SIZE);
        midLayer = new LayerWeb(name_);
    }
    else if(aType.compare("SLV")==0)
    {
        appFactory = nullptr;
        appLayer = appFactory->GetApp();
        prstLayer = new LayerPrst(MAX_DATA_PACKET_SIZE);
        dlLayer = new LayerDL(name_, MAX_DATA_PACKET_SIZE);
        midLayer = new LayerWeb(name_);
    }
    else
    {
        MyThrow ("Unkown adaptor type:%s", aType.c_str());
    }
    // lowerLayer<->dlLayer<->midLayer<->prstLayer<->appLayer
    // dlLayer layer, need lower&upper layer
    // dlLayer->LowerLayer(lowerLayer); set in this->LowerLayer(lowerLayer)
    dlLayer->UpperLayer(midLayer);
    // middle layer, need lower&upper layer
    midLayer->LowerLayer(dlLayer);
    midLayer->UpperLayer(prstLayer);
    // presentation layer, need lower&upper layer
    prstLayer->LowerLayer(midLayer);
    prstLayer->UpperLayer(appLayer);
    // app layer, only need lower layer
    appLayer->LowerLayer(prstLayer);
}

LayerManager::~LayerManager()
{
    if(appFactory)
    {
        delete appFactory;
    }
    if(prstLayer)
    {
        delete prstLayer;
    }
    if(midLayer)
    {
        delete midLayer;
    }
    if(dlLayer)
    {
        delete dlLayer;
    }
}

int LayerManager::Rx(uint8_t * data, int len)
{
    return dlLayer->Rx(data,len);
}

void LayerManager::Clean()
{
    return dlLayer->Clean();
}
