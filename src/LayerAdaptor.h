#ifndef __LAYERMIDDLE_H__
#define __LAYERMIDDLE_H__

#include <cstdint>
#include <string>
#include "ILayer.h"
#include "IOperator.h"

class LayerAdaptor : public ILayer
{
public:
    LayerAdaptor(std::string name, std::string aType, ILayer * lowerLayer);
    ~LayerAdaptor();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void PeriodicRun() override;

    void Clean() override;

    void Release() override;

private:
    ILayer * midLayer;  // middle layer
    ILayer * prstLayer; // presentation layer
    ILayer * appLayer;  // app layer
};

#endif
