#include <stdexcept>
#include "LayerAdaptor.h"
#include "LayerPhcs.h"
#include "LayerWeb.h"

LayerAdaptor::LayerAdaptor(std::string name, std::string aType, ILayer * lowerLayer)
{
    if(aType.compare("PHCS")==0)
    {
        midLayer = new LayerPhcs(name, online);
    }
    else if(aType.compare("WEB")==0)
    {
        midLayer = new LayerWeb(name, online);
    }
    else
    {
        throw std::invalid_argument("Unkown adaptor type:"+aType);
    }
    prstLayer = new TsiSp003Prst();
    appFactory = new AppFactory(online);
    appLayer = appFactory->GetApp();
    // middle layer, need lower&upper layer
    midLayer->LowerLayer(lowerLayer);
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
}

int LayerAdaptor::Rx(uint8_t * data, int len)
{
    int r = midLayer->Rx(data,len);
}

int LayerAdaptor::Tx(uint8_t * data, int len)
{
    return midLayer->Tx(data,len);
}

void LayerAdaptor::PeriodicRun()
{
    return midLayer->PeriodicRun();
}

void LayerAdaptor::Clean()
{
    return midLayer->Clean();
}

void LayerAdaptor::Release()
{
    return midLayer->Release();
}
