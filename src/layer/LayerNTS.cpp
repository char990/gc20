#include <stdexcept>
#include <cstring>
#include <uci/DbHelper.h>
#include <layer/LayerNTS.h>
#include <module/Utils.h>
#include <sign/Controller.h>

using namespace Utils;

const MI_CODE LayerNTS::broadcastMi[BROADCAST_MI_SIZE] = {
    MI_CODE::SystemReset,
    MI_CODE::UpdateTime,
    MI_CODE::EndSession,
    MI_CODE::SignSetTextFrame,
    MI_CODE::SignSetGraphicsFrame,
    MI_CODE::SignSetMessage,
    MI_CODE::SignSetPlan,
    MI_CODE::SignDisplayFrame,
    MI_CODE::SignDisplayMessage,
    MI_CODE::EnablePlan,
    MI_CODE::DisablePlan,
    MI_CODE::SignSetDimmingLevel,
    MI_CODE::PowerOnOff,
    MI_CODE::ResetFaultLog,
    MI_CODE::SignSetHighResolutionGraphicsFrame,
    MI_CODE::SignDisplayAtomicFrames};

LayerNTS::LayerNTS(std::string name_)
{
    name = name_ + ":" + "NTS";
}

LayerNTS::~LayerNTS()
{
}

int LayerNTS::Rx(uint8_t *data, int len)
{
    if (len < 15 || (len & 1) == 0 || data[7] != static_cast<uint8_t>(CTRL_CHAR::STX))
    {
        return 0;
    }
    uint16_t crc1 = Crc::Crc16_1021(data, len - 5);
    uint16_t crc2 = Cnvt::ParseToU16((char *)data + len - 5);
    if (crc1 != crc2)
    {
        return 0;
    }
    int addr = Cnvt::ParseToU8((char *)data + 5);
    UciUser &user = DbHelper::Instance().GetUciUser();
    if (addr != user.DeviceId() || addr == user.BroadcastId())
    {
        return 0;
    }
    _addr = addr;
    int mi = Cnvt::ParseToU8((char *)data + 8);
    if ((mi == static_cast<uint8_t>(MI_CODE::StartSession) && len == 15 && addr == user.DeviceId()) ||
        (sessionTimeout.IsExpired() && session == ISession::SESSION::ON_LINE))
    {
        session = ISession::SESSION::OFF_LINE;
        _ns = 0;
        _nr = 0;
    }
    if (session == ISession::SESSION::ON_LINE)
    {
        if (addr == user.DeviceId())
        { // only matched slave addr inc _nr, for broadcast id ignore ns nr
            int ns = Cnvt::ParseToU8((char *)data + 1);
            int nr = Cnvt::ParseToU8((char *)data + 3);
            if (ns == _nr)
            {
                _nr = IncN(ns);
            }
            else
            {
                if (IncN(ns) == _nr && IncN(nr) == _ns)
                { // master did not get last reply, so _nr & _ns step back
                    _ns = nr;
                }
                else
                { // ns nr not matched
                    MakeNondata(static_cast<uint8_t>(CTRL_CHAR::NAK));
                    lowerLayer->Tx(txbuf, 10);
                    return 0;
                }
            }
        }
        else
        {
            for (int i = 0; i < BROADCAST_MI_SIZE; i++)
            { // check allowed broadcast mi
                if (static_cast<uint8_t>(broadcastMi[i]) == mi)
                {
                    break;
                }
                else
                {
                    if (i == BROADCAST_MI_SIZE - 1)
                    {
                        return 0;
                    }
                }
            }
        }
    }
    else
    {
        if (addr == user.BroadcastId())
        { // ignore broadcast when off-line
            return 0;
        }
    }
    upperLayer->Rx(data + 8, len - 13);
    auto &ctrl = Controller::Instance();
    if (session == ISession::SESSION::ON_LINE)
    {
        sessionTimeout.Setms(user.SessionTimeout() * 1000);
        ctrl.SessionLed(1);
        ctrl.RefreshSessionTime();
        ctrl.RefreshDispTime();
    }
    return 0;
}

bool LayerNTS::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerNTS::Tx(uint8_t *data, int len)
{
    UciUser &user = DbHelper::Instance().GetUciUser();
    if (_addr == user.BroadcastId())
    { // no reply for broadcast
        return 0;
    }
    MakeNondata(static_cast<uint8_t>(CTRL_CHAR::ACK));
    char *p = (char *)txbuf + NON_DATA_PACKET_SIZE;
    *p = static_cast<char>(CTRL_CHAR::SOH);
    p++;
    Cnvt::ParseToAsc(_ns, p);
    p += 2;
    Cnvt::ParseToAsc(_nr, p);
    p += 2;
    Cnvt::ParseToAsc(user.DeviceId(), p);
    p += 2;
    *p = static_cast<char>(CTRL_CHAR::STX);
    p++;
    memcpy(p, data, len);
    EndOfBlock(txbuf + NON_DATA_PACKET_SIZE, len + DATA_PACKET_HEADER_SIZE);
    lowerLayer->Tx(txbuf, NON_DATA_PACKET_SIZE + DATA_PACKET_HEADER_SIZE + len + DATA_PACKET_EOB_SIZE);
    if (session == ISession::SESSION::ON_LINE)
    {
        _ns = IncN(_ns);
    }
    return 0;
}

void LayerNTS::ClrRx()
{
    Session(ISession::SESSION::OFF_LINE);
    upperLayer->ClrRx();
}

void LayerNTS::ClrTx()
{
    lowerLayer->ClrTx();
}

/// -------------------------------------------------------
enum ISession::SESSION LayerNTS::Session()
{
    if (session == ISession::SESSION::ON_LINE)
    {
        if (sessionTimeout.IsExpired())
        {
            Session(ISession::SESSION::OFF_LINE);
        }
    }
    return session;
}

void LayerNTS::Session(enum ISession::SESSION v)
{
    session = v;
    _nr = 0;
    _ns = 0;
    if (v == ISession::SESSION::OFF_LINE)
    {
        sessionTimeout.Clear();
        Controller::Instance().SessionLed(0);
    }
}

/// -------------------------------------------------------
uint8_t LayerNTS::IncN(uint8_t n)
{
    n++;
    return (n == 0) ? 1 : n;
}

void LayerNTS::MakeNondata(uint8_t a)
{
    txbuf[0] = a;
    Cnvt::ParseToAsc(_nr, (char *)txbuf + 1);
    Cnvt::ParseToAsc(DbHelper::Instance().GetUciUser().DeviceId(), (char *)txbuf + 3);
    EndOfBlock(txbuf, 5);
}

void LayerNTS::EndOfBlock(uint8_t *p, int len)
{
    uint16_t crc = Crc::Crc16_1021(p, len);
    Cnvt::ParseU16ToAsc(crc, (char *)p + len);
    *(p + len + 4) = static_cast<uint8_t>(CTRL_CHAR::ETX);
}
