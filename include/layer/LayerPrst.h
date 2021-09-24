#pragma once


#include <layer/ILayer.h>

class LayerPrst : public ILayer
{
public:
    LayerPrst(int maxlen);
    ~LayerPrst();
    
    int Rx(uint8_t * data, int len) override;

    bool IsTxReady() override;
    
    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    int maxlen;
    uint8_t *buf;
};

