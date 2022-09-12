#pragma once

#include <string>
#include <cstdint>
#include <layer/ILayer.h>
#include <module/IOperator.h>
#include <module/OprSp.h>

class LayerSlv : public ILayer
{
public:
    LayerSlv(std::string name_, int groupId, int maxPktSzie);
    ~LayerSlv();

    int Rx(uint8_t *data, int len) override;

    bool IsTxReady() override;

    int Tx(uint8_t *data, int len) override;

    void ClrRx() override;
    void ClrTx() override;

private:
    std::string name;
    int length;
    uint8_t *buf;
    int maxPktSize;
    int groupId;
};
