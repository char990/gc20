#include <layer/UI_LayerManager.h>
#include <layer/LayerNTS.h>
#include <layer/LayerWeb.h>
#include <layer/LayerDL.h>
#include <module/MyDbg.h>
#include <module/Utils.h>

using namespace std;
using namespace Utils;

UI_LayerManager::UI_LayerManager(string name_, string aType)
{
    if(aType.compare("NTS")==0)
    {
        appFactory = new AppFactory();
        appLayer = appFactory->GetApp();
        prstLayer = new LayerPrst(MAX_APP_PACKET_SIZE);
        dlLayer = new LayerDL(name_, POWEROF2_MAX_DATA_PACKET_SIZE);
        LayerNTS *layerNTS = new LayerNTS(name_);
        midLayer = layerNTS;
        //ISession *s = layerNTS;
        appLayer->SetSession((ISession *)layerNTS);
    }
    else if(aType.compare("WEB")==0)
    {
        appFactory = new AppFactory();
        appLayer = appFactory->GetApp();
        prstLayer = new LayerPrst(MAX_APP_PACKET_SIZE);
        dlLayer = new LayerDL(name_, POWEROF2_MAX_DATA_PACKET_SIZE);
        midLayer = new LayerWeb(name_);
    }
    else
    {
        throw invalid_argument(StrFn::PrintfStr("Unkown adaptor type:%s", aType.c_str()));
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

UI_LayerManager::~UI_LayerManager()
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

int UI_LayerManager::Rx(uint8_t * data, int len)
{
    return dlLayer->Rx(data,len);
}

void UI_LayerManager::ClrRx()
{
    return dlLayer->ClrRx();
}
