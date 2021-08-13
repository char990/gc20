#ifndef __LAYERWEB_H__
#define __LAYERWEB_H__
#include <string>
#include <cstdint>
#include <layer/ILayer.h>
#include <module/IOperator.h>

class LayerWeb : public ILayer
{
public:
    LayerWeb(std::string name_);
    ~LayerWeb();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    std::string name;
};

#endif
