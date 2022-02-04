#include <stdexcept>
#include <cstring>
#include <uci/DbHelper.h>
#include <layer/LayerNTS.h>
#include <module/Utils.h>
#include <sign/Controller.h>
#include <gpio/GpioOut.h>
#include <module/MyDbg.h>

using namespace Utils;

const MI::CODE LayerNTS::broadcastMi[BROADCAST_MI_SIZE] = {
    MI::CODE::SystemReset,
    MI::CODE::UpdateTime,
    MI::CODE::EndSession,
    MI::CODE::SignSetTextFrame,
    MI::CODE::SignSetGraphicsFrame,
    MI::CODE::SignSetMessage,
    MI::CODE::SignSetPlan,
    MI::CODE::SignDisplayFrame,
    MI::CODE::SignDisplayMessage,
    MI::CODE::EnablePlan,
    MI::CODE::DisablePlan,
    MI::CODE::SignSetDimmingLevel,
    MI::CODE::PowerOnOff,
    MI::CODE::ResetFaultLog,
    MI::CODE::SignSetHighResolutionGraphicsFrame,
    MI::CODE::SignDisplayAtomicFrames};

std::vector<LayerNTS *> LayerNTS::storage;

OprSp *LayerNTS::monitor = nullptr;

LayerNTS::LayerNTS(std::string name_)
{
    name = name_ + ":" + "NTS";
    storage.push_back(this);
}

LayerNTS::~LayerNTS()
{
}

int LayerNTS::Rx(uint8_t *data, int len)
{
    int addr = Cnvt::ParseToU8((char *)data + 5);
    UciUser &user = DbHelper::Instance().GetUciUser();
    if (addr != user.DeviceId() || addr == user.BroadcastId())
    {
        PrintDbg(DBG_LOG, "NOT my addr: %d", addr);
        return 0;
    }
    if (data[7] != static_cast<uint8_t>(CTRL_CHAR::STX))
    {
        PrintDbg(DBG_LOG, "Missing STX at [7]");
        return 0;
    }
    if (len < 15 || (len & 1) == 0)
    {
        uint8_t data[3];
        data[0] = 0;
        data[1] = Cnvt::ParseToU8((char *)data + 8);
        data[2] = 3;
        char dst[6];
        Cnvt::ParseToAsc(data, dst, 3);
        Tx((uint8_t *)dst, 6);
        return 0;
    }
    uint16_t crc1 = Crc::Crc16_1021(data, len - 5);
    uint16_t crc2 = Cnvt::ParseToU16((char *)data + len - 5);
    if (crc1 != crc2)
    {
        MakeNondata(static_cast<uint8_t>(CTRL_CHAR::NAK));
        LowerLayerTx(txbuf, 10);
        return 0;
    }
    _addr = addr;
    int mi = Cnvt::ParseToU8((char *)data + 8);
    if ((mi == static_cast<uint8_t>(MI::CODE::StartSession) && len == 15 && addr == user.DeviceId()) ||
        (ntsSessionTimeout.IsExpired() && session == ISession::SESSION::ON_LINE))
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
                    LowerLayerTx(txbuf, 10);
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
                    break; // this is a valid broadcast command
                }
                else
                {
                    if (i == BROADCAST_MI_SIZE - 1)
                    {
                        PrintDbg(DBG_LOG, "Unsupported broadcast cmd:%d", mi);
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
            PrintDbg(DBG_LOG, "Ignore broadcast when off-line:cmd=%d", mi);
            return 0;
        }
    }
    upperLayer->Rx(data + 8, len - 13);
    auto &ctrl = Controller::Instance();
    if (session == ISession::SESSION::ON_LINE)
    {
        long ms = user.SessionTimeout();
        if (ms == 0)
        {
            ntsSessionTimeout.Clear();
        }
        else
        {
            ntsSessionTimeout.Setms(ms * 1000);
        }
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
    LowerLayerTx(txbuf, NON_DATA_PACKET_SIZE + DATA_PACKET_HEADER_SIZE + len + DATA_PACKET_EOB_SIZE);
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
    if (monitor != nullptr)
    {
        monitor->ClrTx();
    }
}

/// -------------------------------------------------------
enum ISession::SESSION LayerNTS::Session()
{
    /*
    if (session == ISession::SESSION::ON_LINE)
    {
        if (ntsSessionTimeout.IsExpired())
        {
            Session(ISession::SESSION::OFF_LINE);
        }
    }*/
    return session;
}

void LayerNTS::Session(enum ISession::SESSION v)
{
    session = v;
    _nr = 0;
    _ns = 0;
    if (v == ISession::SESSION::OFF_LINE)
    {
        ntsSessionTimeout.Clear();
        if (!pPinStatusLed->GetPin() && !IsAnyOnline())
        {
            pPinStatusLed->SetPinHigh();
        }
    }
    else if (v == ISession::SESSION::ON_LINE && pPinStatusLed->GetPin())
    {
        pPinStatusLed->SetPinLow();
    }
}

bool LayerNTS::IsThisSessionTimeout()
{
    return ((session == ISession::SESSION::ON_LINE) && (ntsSessionTimeout.IsExpired()));
}

bool LayerNTS::IsAnySessionTimeout()
{
    for (auto &s : storage)
    {
        if (s->IsThisSessionTimeout())
        {
            return true;
        }
    }
    return false;
}

bool LayerNTS::IsAnyOnline()
{
    for (auto &s : storage)
    {
        if (s->Session() == ISession::SESSION::ON_LINE)
        {
            return true;
        }
    }
    return false;
}

void LayerNTS::ClearAllSessionTimeout()
{
    for (auto &s : storage)
    {
        if (s->Session() != ISession::SESSION::OFF_LINE)
        {
            s->Session(ISession::SESSION::OFF_LINE);
        }
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

int LayerNTS::LowerLayerTx(uint8_t *buf, int len)
{
    if (monitor != nullptr)
    {
        monitor->Tx(buf, len);
    }
    return lowerLayer->Tx(buf, len);
}
