#include <stdexcept>
#include "LayerAdaptor.h"
#include "LayerNTS.h"
#include "LayerWeb.h"
#include "LayerDL.h"

LayerAdaptor::LayerAdaptor(std::string name_, std::string aType)
{
    appFactory = new AppFactory();
    appLayer = appFactory->GetApp();
    prstLayer = new TsiSp003Prst();
    dlLayer = new LayerDL(name_, MAX_DATA_PACKET_SIZE);
    if(aType.compare("NTS")==0)
    {
        midLayer = new LayerNTS(name_, appLayer);
    }
    else if(aType.compare("WEB")==0)
    {
        midLayer = new LayerWeb(name_, appLayer);
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

LayerAdaptor::~LayerAdaptor()
{
    delete appFactory;
    delete prstLayer;
    delete midLayer;
    delete dlLayer;
}

int LayerAdaptor::Rx(uint8_t * data, int len)
{
    int r = dlLayer->Rx(data,len);
}

void LayerAdaptor::Clean()
{
    return dlLayer->Clean();
}
