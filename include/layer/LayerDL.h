#ifndef __LAYERPACKET_H__
#define __LAYERPACKET_H__
#include <string>
#include <cstdint>
#include <layer/ILayer.h>
#include <tsisp003/TsiSp003Const.h>

/// \brief Data link layer: Pick up a data packet which starts with SOH and ends with ETX
class LayerDL : public ILayer
{
public:
    LayerDL(std::string name_, int size);
    ~LayerDL();

    int Rx(uint8_t * data, int len) override;

    bool IsTxReady() override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    std::string name;
    uint8_t *buf;
    int size;
    int length;
};
#endif
