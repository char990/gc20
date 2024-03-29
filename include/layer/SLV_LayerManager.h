#pragma once

#include <cstdint>
#include <string>
#include <layer/LayerSlv.h>
#include <layer/LayerPrst.h>
#include <sign/Group.h>
#include <module/OprSp.h>

class SLV_LayerManager : public IUpperLayer
{
public:
    SLV_LayerManager(std::string name_, int groupId, IUpperLayer *upperLayer);
    ~SLV_LayerManager();

    int Rx(uint8_t *data, int len) override;

    void ClrRx() override;

    virtual void LowerLayer(ILowerLayer *lowerLayer)
    {
        dlLayer->LowerLayer(lowerLayer);
    }

protected:
    LayerSlv *dlLayer{nullptr};
    LayerPrst *prstLayer{nullptr}; // presentation layer
    IUpperLayer *appLayer{nullptr};
    int maxPktSize;
};
