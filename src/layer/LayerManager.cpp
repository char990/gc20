#include <stdexcept>
#include <layer/LayerManager.h>
#include <layer/LayerNTS.h>
#include <layer/LayerWeb.h>
#include <layer/LayerDL.h>

LayerManager::LayerManager(std::string name_, std::string aType)
{
    appFactory = new AppFactory();
    appLayer = appFactory->GetApp();
    prstLayer = new TsiSp003Prst();
    dlLayer = new LayerDL(name_, MAX_DATA_PACKET_SIZE);
    if(aType.compare("NTS")==0)
    {
        LayerNTS *layerNTS = new LayerNTS(name_);
        midLayer = layerNTS;
        //ISession *s = layerNTS;
        appLayer->SetSession((ISession *)layerNTS);
    }
    else if(aType.compare("WEB")==0)
    {
        midLayer = new LayerWeb(name_);
    }
    else
    {
        throw std::invalid_argument("Unkown adaptor type:"+aType);
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
    delete appFactory;
    delete prstLayer;
    delete midLayer;
    delete dlLayer;
}

int LayerManager::Rx(uint8_t * data, int len)
{
    return dlLayer->Rx(data,len);
}

void LayerManager::Clean()
{
    return dlLayer->Clean();
}
