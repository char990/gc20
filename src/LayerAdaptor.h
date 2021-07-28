#ifndef __LAYERMIDDLE_H__
#define __LAYERMIDDLE_H__

#include <cstdint>
#include <string>
#include "TsiSp003Prst.h"
#include "TsiSp003App.h"
#include "AppFactory.h"


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
    TsiSp003Prst * prstLayer; // presentation layer
    TsiSp003App * appLayer;  // app layer
    AppFactory * appFactory;
    bool online;
};

#endif
