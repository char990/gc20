#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <layer/ILayer.h>
#include <tsisp003/TsiSp003Const.h>

/// \brief Data link layer: Pick up a data packet which starts with SOH and ends with ETX
class LayerDtLk : public ILayer
{
public:
    LayerDtLk(std::string name_, int size);
    ~LayerDtLk();

    int Rx(uint8_t *data, int len) override;

    bool IsTxReady() override;

    int Tx(uint8_t *data, int len) override;

    void ClrRx() override;

    void ClrTx() override;

    void PrintRxBuf() override { lowerLayer->PrintRxBuf(); };

private:
    std::string name;
    uint8_t *buf;
    int maxPktSize;
    int length;
};
