#ifndef __PRESENTAIONLAYER_H__
#define __PRESENTAIONLAYER_H__

#include "ILayer.h"

class TsiSp003Prst : public ILayer
{
public:
    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void PeriodicRun() override;

    void Clean() override;

    void Release() override;

private:
    static const int BUF_SIZE = 128*1024+256;
    uint8_t buf[BUF_SIZE];
};

#endif
