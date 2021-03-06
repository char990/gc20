#pragma once


#include <cstdint>
#include <string>
#include <layer/LayerDL.h>
#include <layer/LayerPrst.h>
#include <tsisp003/TsiSp003App.h>
#include <tsisp003/AppFactory.h>


class UI_LayerManager : public IUpperLayer
{
public:
    UI_LayerManager(std::string name_, std::string aType);
    ~UI_LayerManager();

    int Rx(uint8_t * data, int len) override;

    void ClrRx() override;

    virtual void LowerLayer(ILowerLayer * lowerLayer) override
    {
        dlLayer->LowerLayer(lowerLayer);
    }

private:
    LayerDL * dlLayer{nullptr};
    ILayer * midLayer{nullptr};  // middle layer: NTS or Web
    LayerPrst * prstLayer{nullptr}; // presentation layer
    TsiSp003App * appLayer{nullptr};  // app layer
    AppFactory * appFactory{nullptr};
};
