#pragma once


#include <layer/ILayer.h>

class LayerPrst : public ILayer
{
public:
    LayerPrst(int maxlen);  // maxlen is the length of Hex packet
    ~LayerPrst();
    
    int Rx(uint8_t * data, int len) override;

    bool IsTxReady() override;
    
    int Tx(uint8_t * data, int len) override;

    void ClrRx() override;

    void ClrTx() override;

    void PrintRxBuf() override { lowerLayer->PrintRxBuf();};

private:
    int maxlen;
    uint8_t *buf;
};

