#pragma once

#include <string>
#include <cstdint>
#include <layer/ILayer.h>
#include <module/IOperator.h>
#include <module/OprSp.h>

class LayerWeb : public ILayer
{
public:
    LayerWeb(std::string name_);
    ~LayerWeb();

    static OprSp * monitor;

    int Rx(uint8_t * data, int len) override;

    bool IsTxReady() override;

    int Tx(uint8_t * data, int len) override;

    void ClrRx() override;
    void ClrTx() override;

private:
    std::string name;
};


