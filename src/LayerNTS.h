#ifndef __LAYERPHCS_H__
#define __LAYERPHCS_H__

#include <unistd.h>
#include <string>

#include "BootTimer.h"
#include "ILayer.h"
#include "IOnline.h"
#include "TsiSp003Const.h"

/// \brief  LayerNTS is Network+Transport+Session Layer of TsiSp003
///         It's the upper of DataLink Layer and lower of Presentation Layer
///         With Interface IOnline, it can get online status of Application Layer
class LayerNTS : public ILayer
{
public:
    LayerNTS(std::string name_, IOnline * online);
    ~LayerNTS();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

private:
    std::string name;
    IOnline * online;

    /// \brief Session timeout timer
    BootTimer sessionTimeout;

    /// \brief protocol fields 
    uint8_t _addr;
    uint8_t _nr, _ns;
    uint8_t IncN(uint8_t n);
    void MakeNondata(uint8_t a);
    void EndOfBlock(uint8_t *p, int len);

    int rcvd;

    uint8_t buf[MAX_ACK_DATA_PACKET_SIZE];
};

#endif
