#include <stdexcept>
#include <cstring>
#include "DbHelper.h"
#include "LayerNTS.h"
#include "Utilities.h"

using namespace Util;

LayerNTS::LayerNTS(std::string name_, IOnline *online)
    : online(online)
{
    name = name_ + ":" + "NTS";
    sessionTimeout.Clear();
}

LayerNTS::~LayerNTS()
{
}

int LayerNTS::Rx(uint8_t *data, int len)
{
    if(len<15 || (len&1)==0 || buf[7]!=CTRL_CHAR::STX)
    {
        return 0;
    }
    uint16_t crc1 = Crc::Crc16_1021(buf,len-5);
    uint16_t crc2 = Cnvt::ParseToU16(buf+len-5);
    if(crc1!=crc2)
    {
        return 0;
    }
    int addr = Cnvt::ParseToU8(buf+5);
    DbHelper & db = DbHelper::Instance();
    if(addr!=db.DeviceId() || addr!=db.BroadcastId())
    {
        return 0;
    }
    _addr=addr;
    int mi = Cnvt::ParseToU8(buf+8);
    if(mi == MI_CODE::StartSession || sessionTimeout.IsExpired())
    {
        online->Online(false);
        sessionTimeout.Clear();
    }
    if(online->Online())
    {
        if(addr==db.DeviceId())
        {// only matched slave addr inc _nr, for broadcast id ignore ns nr
            int ns = Cnvt::ParseToU8(buf+1);
            int nr = Cnvt::ParseToU8(buf+3);
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
                    MakeNondata(CTRL_CHAR::NAK);
                    lowerLayer->Tx(buf, 10);
                    return 0;
                }
            }
        }
    }
    else
    {
        if(addr==db.BroadcastId())
        {// ignore broadcast when off-line
            return 0;
        }
        _ns=0;_nr=0;
    }
    upperLayer->Rx(buf+8, len-13);
    if (online->Online())
    {
        sessionTimeout.Setms(db.SessionTimeout()*1000);
    }
    return 0;
}

int LayerNTS::Tx(uint8_t *data, int len)
{
    if(_addr==DbHelper::Instance().BroadcastId())
    {// no reply for broadcast
        return 0;
    }
    MakeNondata(CTRL_CHAR::ACK);
    uint8_t *p=buf+NON_DATA_PACKET_SIZE;
    *p=CTRL_CHAR::SOH; p++;
    Cnvt::ParseToAsc(_ns,p); p+=2;
    Cnvt::ParseToAsc(_nr,p); p+=2;
    Cnvt::ParseToAsc(DbHelper::Instance().DeviceId(),p); p+=2;
    *p=CTRL_CHAR::STX; p++;
    memcpy(p,data,len);
    EndOfBlock(buf+NON_DATA_PACKET_SIZE, len+DATA_PACKET_HEADER_SIZE);
    _ns=IncN(_ns);
    return lowerLayer->Tx(buf, NON_DATA_PACKET_SIZE+DATA_PACKET_HEADER_SIZE+len+DATA_PACKET_EOB_SIZE);
}

void LayerNTS::Clean()
{
    _nr = 0;
    _ns = 0;
    upperLayer->Clean();
    sessionTimeout.Setms(0);
}

/// -------------------------------------------------------
uint8_t LayerNTS::IncN(uint8_t n)
{
    n++;
    return (n == 0) ? 1 : n ;
}

void LayerNTS::MakeNondata(uint8_t a)
{
    buf[0]=a;
    Cnvt::ParseToAsc(_nr,buf+1);
    Cnvt::ParseToAsc(DbHelper::Instance().DeviceId(),buf+3);
    EndOfBlock(buf, 5);
}

void LayerNTS::EndOfBlock(uint8_t *p, int len)
{
    uint16_t crc = Crc::Crc16_1021(p,len);
    p+=len;
    Cnvt::ParseToAsc(crc>>8,p);
    p+=2;
    Cnvt::ParseToAsc(crc,p);
    p+=2;
    *p=CTRL_CHAR::ETX;
}
