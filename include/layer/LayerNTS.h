#ifndef __LAYERPHCS_H__
#define __LAYERPHCS_H__

#include <unistd.h>
#include <string>

#include <module/BootTimer.h>
#include <layer/ILayer.h>
#include <layer/ISession.h>
#include <tsisp003/TsiSp003Const.h>

/// \brief  LayerNTS is Network+Transport+Session Layer of TsiSp003
///         It's the upper of DataLink Layer and lower of Presentation Layer
///         With Interface IOnline, it can get online status of Application Layer
class LayerNTS : public ILayer, public ISession
{
public:
    LayerNTS(std::string name_);
    ~LayerNTS();

    int Rx(uint8_t * data, int len) override;

    int Tx(uint8_t * data, int len) override;

    void Clean() override;

    enum ISession::SESSION Session() override;
    void Session(enum ISession::SESSION v) override;

    uint8_t Seed() override  { return seed;};
    void Seed(uint8_t v) override { seed = v ; };

private:
    std::string name;
    enum ISession::SESSION session;
    uint8_t seed;

    /// \brief Session timeout timer
    BootTimer sessionTimeout;

    /// \brief protocol fields 
    uint8_t _addr;
    uint8_t _nr, _ns;
    uint8_t IncN(uint8_t n);
    void MakeNondata(uint8_t a);
    void EndOfBlock(uint8_t *p, int len);
    int rcvd;
    uint8_t txbuf[MAX_ACK_DATA_PACKET_SIZE];
    #define BROADCAST_MI_SIZE 16
    static const uint8_t broadcastMi[BROADCAST_MI_SIZE];
};

#endif
