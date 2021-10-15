#include <stdexcept>
#include <cstring>
#include <uci/DbHelper.h>
#include <layer/LayerNTS.h>
#include <module/Utils.h>
#include <layer/StatusLed.h>
#include <sign/Controller.h>

using namespace Utils;

BootTimer LayerNTS::sessionTimeout;

const uint8_t LayerNTS::broadcastMi[BROADCAST_MI_SIZE]={
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
    MI::CODE::SignDisplayAtomicFrames
};

LayerNTS::LayerNTS(std::string name_)
{
    name = name_ + ":" + "NTS";
}

LayerNTS::~LayerNTS()
{
}

int LayerNTS::Rx(uint8_t *data, int len)
{
    StatusLed::Instance().ReloadDataSt();
    if(len<15 || (len&1)==0 || data[7]!=DATALINK::CTRL_CHAR::STX)
    {
        return 0;
    }
    uint16_t crc1 = Crc::Crc16_1021(data,len-5);
    uint16_t crc2 = Cnvt::ParseToU16((char *)data+len-5);
    if(crc1!=crc2)
    {
        return 0;
    }
    int addr = Cnvt::ParseToU8((char *)data+5);
    UciUser & user = DbHelper::Instance().GetUciUser();
    if(addr!=user.DeviceId() || addr==user.BroadcastId())
    {
        return 0;
    }
    _addr=addr;
    int mi = Cnvt::ParseToU8((char *)data+8);
    auto sessionT = sessionTimeout.IsExpired();
    if((mi == MI::CODE::StartSession && len==15 && addr==user.DeviceId()) || sessionT)
    {
        session=ISession::SESSION::OFF_LINE;
        _ns=0;_nr=0;
        if(sessionT)
        {
            PrintDbg("SessionTimeout!!!\n");
            sessionTimeout.Clear();
            Controller::Instance().ctrllerError.Push(DEV::ERROR::CommunicationsTimeoutError, 1);
        }
    }
    if(session==ISession::SESSION::ON_LINE)
    {
        if(addr==user.DeviceId())
        {// only matched slave addr inc _nr, for broadcast id ignore ns nr
            int ns = Cnvt::ParseToU8((char *)data+1);
            int nr = Cnvt::ParseToU8((char *)data+3);
            if(ns==_nr)
            {
                _nr = IncN(ns);
            }
            else
            {
                if(IncN(ns)==_nr && IncN(nr)==_ns)
                {// master did not get last reply, so _nr & _ns step back
                    _ns=nr;
                }
                else
                {// ns nr not matched
                    MakeNondata(DATALINK::CTRL_CHAR::NAK);
                    lowerLayer->Tx(txbuf, 10);
                    return 0;
                }
            }
        }
        else
        {
            for(int i=0;i<BROADCAST_MI_SIZE;i++)
            {// check allowed broadcast mi
                if(broadcastMi[i]==mi)
                {
                    break;
                }
                else
                {
                    if(i==BROADCAST_MI_SIZE-1)
                    {
                        return 0;
                    }
                }
            }
        }
    }
    else
    {
        if(addr==user.BroadcastId())
        {// ignore broadcast when off-line
            return 0;
        }
    }
    upperLayer->Rx(data+8, len-13);
    if(session==ISession::SESSION::ON_LINE)
    {
        PrintDbg("Reload SessionTimeout\n");
        sessionTimeout.Setms(user.SessionTimeout()*1000);
        Controller::Instance().ctrllerError.Push(DEV::ERROR::CommunicationsTimeoutError, 0);
    }
    return 0;
}

bool LayerNTS::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerNTS::Tx(uint8_t *data, int len)
{
    StatusLed::Instance().ReloadDataSt();
    UciUser & user = DbHelper::Instance().GetUciUser();
    if(_addr==user.BroadcastId())
    {// no reply for broadcast
        return 0;
    }
    MakeNondata(DATALINK::CTRL_CHAR::ACK);
    char *p=(char *)txbuf+NON_DATA_PACKET_SIZE;
    *p=DATALINK::CTRL_CHAR::SOH; p++;
    Cnvt::ParseToAsc(_ns,p); p+=2;
    Cnvt::ParseToAsc(_nr,p); p+=2;
    Cnvt::ParseToAsc(user.DeviceId(),p); p+=2;
    *p=DATALINK::CTRL_CHAR::STX; p++;
    memcpy(p,data,len);
    EndOfBlock(txbuf+NON_DATA_PACKET_SIZE, len+DATA_PACKET_HEADER_SIZE);
    lowerLayer->Tx(txbuf, NON_DATA_PACKET_SIZE+DATA_PACKET_HEADER_SIZE+len+DATA_PACKET_EOB_SIZE);
    if(session==ISession::SESSION::ON_LINE)
    {
        _ns=IncN(_ns);
    }
    return 0;
}

void LayerNTS::ClrRx()
{
    _nr = 0;
    _ns = 0;
    upperLayer->ClrRx();
}

void LayerNTS::ClrTx()
{
    lowerLayer->ClrTx();
}

/// -------------------------------------------------------
enum ISession::SESSION LayerNTS::Session()
{
    return session;
}

void LayerNTS::Session(enum ISession::SESSION v)
{
    session = v;
    _nr = 0;
    _ns = 0;
}

/// -------------------------------------------------------
uint8_t LayerNTS::IncN(uint8_t n)
{
    n++;
    return (n == 0) ? 1 : n ;
}

void LayerNTS::MakeNondata(uint8_t a)
{
    txbuf[0]=a;
    Cnvt::ParseToAsc(_nr, (char*)txbuf+1);
    Cnvt::ParseToAsc(DbHelper::Instance().GetUciUser().DeviceId(), (char*)txbuf+3);
    EndOfBlock(txbuf, 5);
}

void LayerNTS::EndOfBlock(uint8_t *p, int len)
{
    uint16_t crc = Crc::Crc16_1021(p, len);
    Cnvt::ParseU16ToAsc(crc, (char*)p+len);
    *(p+len+4) = DATALINK::CTRL_CHAR::ETX;
}
