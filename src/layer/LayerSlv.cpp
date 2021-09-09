#include <cstdio>
#include <unistd.h>

#include <layer/LayerSlv.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>

using namespace Utils;

LayerSlv::LayerSlv(std::string name_)
{
    name = name_ + ":" + "SLV";
}

LayerSlv::~LayerSlv()
{
}

int LayerSlv::Rx(uint8_t * data, int len)
{
    #if 0
    if(len<15 || (len&1)==0 || data[7]!=DATALINK::CTRL_CHAR::STX)
    {
        return 0;
    }
    uint16_t crc1 = Crc::Crc16_8005(data,len-5);
    uint16_t crc2 = Cnvt::ParseToU16((char *)data+len-5);
    if(crc1!=crc2)
    {
        return 0;
    }
    int addr = Cnvt::ParseToU8((char *)data+5);
    UciUser & cfg = DbHelper::Instance().uciUser;
    if(addr!=cfg.DeviceId() || addr==cfg.BroadcastId())
    {
        return 0;
    }
    _addr=addr;
    int mi = Cnvt::ParseToU8((char *)data+8);
    if((mi == MI::CODE::StartSession && len==15 && addr==cfg.DeviceId()) ||
        sessionTimeout.IsExpired())
    {
        session=ISession::SESSION::OFF_LINE;
        sessionTimeout.Clear();
        _ns=0;_nr=0;
    }
    if(session==ISession::SESSION::ON_LINE)
    {
        if(addr==cfg.DeviceId())
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
        if(addr==cfg.BroadcastId())
        {// ignore broadcast when off-line
            return 0;
        }
    }
    upperLayer->Rx(data+8, len-13);
    if(session==ISession::SESSION::ON_LINE)
    {
        sessionTimeout.Setms(cfg.SessionTimeout()*1000);
    }
    #endif
    return 0;
}

bool LayerSlv::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerSlv::Tx(uint8_t * data, int len)
{
    #if 0
    StatusLed::Instance().ReloadDataSt();
    UciUser & cfg = DbHelper::Instance().uciUser;
    if(_addr==cfg.BroadcastId())
    {// no reply for broadcast
        return 0;
    }
    MakeNondata(DATALINK::CTRL_CHAR::ACK);
    char *p=(char *)txbuf+NON_DATA_PACKET_SIZE;
    *p=DATALINK::CTRL_CHAR::SOH; p++;
    Cnvt::ParseToAsc(_ns,p); p+=2;
    Cnvt::ParseToAsc(_nr,p); p+=2;
    Cnvt::ParseToAsc(cfg.DeviceId(),p); p+=2;
    *p=DATALINK::CTRL_CHAR::STX; p++;
    memcpy(p,data,len);
    uint16_t crc = Crc::Crc16_1021(p,len);
    p+=len;
    Cnvt::ParseToAsc(crc>>8,(char *)p);
    p+=2;
    Cnvt::ParseToAsc(crc,(char *)p);
    p+=2;
    *p=DATALINK::CTRL_CHAR::ETX;


    EndOfBlock(txbuf+NON_DATA_PACKET_SIZE, len+DATA_PACKET_HEADER_SIZE);
    lowerLayer->Tx(txbuf, NON_DATA_PACKET_SIZE+DATA_PACKET_HEADER_SIZE+len+DATA_PACKET_EOB_SIZE);
    if(session==ISession::SESSION::ON_LINE)
    {
        _ns=IncN(_ns);
    }
    #endif
    return 0;
}

void LayerSlv::Clean()
{
    upperLayer->Clean();
}
