#ifndef __LAYERSLV_H__
#define __LAYERSLV_H__
#include <string>
#include <cstdint>
#include <layer/ILayer.h>
#include <module/IOperator.h>

class LayerSlv : public ILayer
{
public:
    LayerSlv(std::string name_);
    ~LayerSlv();

    int Rx(uint8_t * data, int len) override;

    bool IsTxReady() override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    std::string name;
};

#endif
