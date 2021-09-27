#pragma once

#include <cstdint>
#include <string>
#include <layer/LayerSlv.h>
#include <layer/LayerPrst.h>
#include <sign/Group.h>


class SLV_LayerManager : public IUpperLayer
{
public:
    SLV_LayerManager(std::string name_,  IUpperLayer * upperLayer);
    ~SLV_LayerManager();

    int Rx(uint8_t * data, int len) override;

    void Clean() override;

    virtual void LowerLayer(ILowerLayer * lowerLayer)
    {
        dlLayer->LowerLayer(lowerLayer);
    }

private:
    LayerSlv * dlLayer;
    LayerPrst * prstLayer; // presentation layer
    IUpperLayer * appLayer; 
    int maxPktSize;
};

