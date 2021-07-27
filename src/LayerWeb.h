#ifndef __WEB2APPADAPTOR_H__
#define __WEB2APPADAPTOR_H__
#include <string>
#include "ILayer.h"
#include "IOperator.h"

class LayerWeb : public ILayer
{
public:
    LayerWeb(std::string name);
    ~LayerWeb();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void PeriodicRun() override;

    void Clean() override;

    void Release() override;

private:
    std::string name;
};

#endif
