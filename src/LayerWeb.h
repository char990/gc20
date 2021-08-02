#ifndef __LAYERWEB_H__
#define __LAYERWEB_H__
#include <string>
#include "ILayer.h"
#include "IOperator.h"
#include "IOnline.h"

class LayerWeb : public ILayer
{
public:
    LayerWeb(std::string name_, IOnline * online);
    ~LayerWeb();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    std::string name;
    IOnline * online;
};

#endif
