#ifndef __LAYERMIDDLE_H__
#define __LAYERMIDDLE_H__

#include <cstdint>
#include <string>
#include <layer/LayerDL.h>
#include <layer/LayerPrst.h>
#include <tsisp003/TsiSp003App.h>
#include <tsisp003/AppFactory.h>


class LayerManager : public IUpperLayer
{
public:
    LayerManager(std::string name_, std::string aType);
    ~LayerManager();

    int Rx(uint8_t * data, int len) override;

    void Clean() override;

    virtual void LowerLayer(ILowerLayer * lowerLayer)
    {
        dlLayer->LowerLayer(lowerLayer);
    }

private:
    LayerDL * dlLayer;
    ILayer * midLayer;  // middle layer: NTS or Web
    LayerPrst * prstLayer; // presentation layer
    TsiSp003App * appLayer;  // app layer
    AppFactory * appFactory;
};

#endif
