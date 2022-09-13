#pragma once


#include <unistd.h>
#include <string>
#include <vector>

#include <module/BootTimer.h>
#include <layer/ILayer.h>
#include <layer/ISession.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/OprSp.h>

/// \brief  LayerNTS is Network+Transport+Session Layer of TsiSp003
///         It's the upper of DataLink Layer and lower of Presentation Layer
///         With Interface ISession, it can get online status of Application Layer
class LayerNTS : public ILayer, public ISession
{
public:
    LayerNTS(std::string name_);
    ~LayerNTS();
    
    static OprSp * monitor;

    static std::vector<LayerNTS *> storage; 
    static bool IsAnySessionTimeout();
    static void ClearAllSessionTimeout();
    static bool IsAnyOnline();

    int Rx(uint8_t * data, int len) override;

    bool IsTxReady() override;

    int Tx(uint8_t * data, int len) override;

    void ClrRx() override;
    void ClrTx() override;

    enum ISession::SESSION Session() override;
    void Session(enum ISession::SESSION v) override;
    bool IsThisSessionTimeout();

    uint8_t Seed() override  { return seed;};
    void Seed(uint8_t v) override { seed = v ; };

private:
    std::string name;

    enum ISession::SESSION session;
    uint8_t seed;
    BootTimer ntsSessionTimeout;

    /// \brief protocol fields 
    uint8_t _addr;
    uint8_t _nr{0}, _ns{0};
    uint8_t IncN(uint8_t n);
    void MakeNondata(uint8_t a);
    void EndOfBlock(uint8_t *p, int len);
    uint8_t txbuf[MAX_ACK_DATA_PACKET_SIZE];
    #define BROADCAST_MI_SIZE 16
    static const MI::CODE broadcastMi[BROADCAST_MI_SIZE];

    int LowerLayerTx(uint8_t * buf, int len);
};

