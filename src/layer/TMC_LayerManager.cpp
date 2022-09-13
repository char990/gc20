#include <layer/TMC_LayerManager.h>
#include <layer/LayerNTS.h>
#include <layer/LayerDtLk.h>
#include <module/MyDbg.h>
#include <module/Utils.h>

using namespace std;
using namespace Utils;

TMC_LayerManager::TMC_LayerManager(string name_)
{
    appFactory = new AppFactory();
    appLayer = appFactory->GetApp();
    appLayer->SetName(name_);
    prstLayer = new LayerPrst(MAX_APP_PACKET_SIZE);
    dlLayer = new LayerDtLk(name_, POWEROF2_MAX_DATA_PACKET_SIZE);
    LayerNTS *layerNTS = new LayerNTS(name_);
    midLayer = layerNTS;
    appLayer->SetSession((ISession *)layerNTS);

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

TMC_LayerManager::~TMC_LayerManager()
{
    if (appFactory)
    {
        delete appFactory;
    }
    if (prstLayer)
    {
        delete prstLayer;
    }
    if (midLayer)
    {
        delete midLayer;
    }
    if (dlLayer)
    {
        delete dlLayer;
    }
}

int TMC_LayerManager::Rx(uint8_t *data, int len)
{
    return dlLayer->Rx(data, len);
}

void TMC_LayerManager::ClrRx()
{
    return dlLayer->ClrRx();
}
