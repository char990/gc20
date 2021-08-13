#ifndef __PRESENTAIONLAYER_H__
#define __PRESENTAIONLAYER_H__

#include <layer/ILayer.h>
#include <tsisp003/TsiSp003Const.h>

class TsiSp003Prst : public ILayer
{
public:
    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    uint8_t buf[MAX_APP_PACKET_SIZE*2];
};

#endif
