#pragma once


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

    bool IsTxReady() override;

    int Tx(uint8_t * data, int len) override;

    void ClrRx() override;
    void ClrTx() override;

    enum ISession::SESSION Session() override;
    void Session(enum ISession::SESSION v) override;

    uint8_t Seed() override  { return seed;};
    void Seed(uint8_t v) override { seed = v ; };

private:
    std::string name;
    enum ISession::SESSION session{ISession::SESSION::OFF_LINE};
    uint8_t seed;
    BootTimer sessionTimeout;

    /// \brief protocol fields 
    uint8_t _addr;
    uint8_t _nr{0}, _ns{0};
    uint8_t IncN(uint8_t n);
    void MakeNondata(uint8_t a);
    void EndOfBlock(uint8_t *p, int len);
    uint8_t txbuf[MAX_ACK_DATA_PACKET_SIZE];
    #define BROADCAST_MI_SIZE 16
    static const uint8_t broadcastMi[BROADCAST_MI_SIZE];
};

