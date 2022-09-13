#include <unistd.h>
#include <cstdio>
#include <cstring>

#include <layer/LayerSlv.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/QueueLtd.h>

QueueLtd * qltdSlave;

using namespace Utils;

LayerSlv::LayerSlv(std::string name_, int groupId, int maxPktSize)
    : length(0), maxPktSize(maxPktSize)
{
    this->groupId = groupId;
    name = name_ + ":" + "SLV";
    buf = new uint8_t[maxPktSize];
    qltdSlave = new QueueLtd(DbHelper::Instance().GetUciHardware().TestSlave());
}

LayerSlv::~LayerSlv()
{
    delete[] buf;
    delete qltdSlave;
}

int LayerSlv::Rx(uint8_t *data, int len)
{
    uint8_t *p = data;
    for (int i = 0; i < len; i++)
    {
        uint8_t c = *p++;
        if (c == static_cast<uint8_t>(CTRL_CHAR::STX))
        { // packet start, clear buffer
            buf[0] = c;
            length = 1;
        }
        else
        {
            if (length > 0)
            {
                if (length < maxPktSize)
                {
                    buf[length++] = c;
                    if (c == static_cast<uint8_t>(CTRL_CHAR::ETX))
                    {
                        if (len >= 34 && (len & 1) == 0)
                        {
                            uint16_t crc1 = Crc::Crc16_8005(buf, len - 5);
                            uint16_t crc2 = Cnvt::ParseToU16((char *)buf + len - 5);
                            if (crc1 == crc2)
                            {
                                upperLayer->Rx(buf + 1, length - 6);
                            }
                            qltdSlave->Push(groupId+'0', buf, length, 1);
                        }
                        length = 0;
                        return 0; // only deal with one pkt. Discard other data.
                    }
                }
                else
                {
                    length = 0;
                }
            }
        }
    }
    return 0;
}

bool LayerSlv::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerSlv::Tx(uint8_t *data, int len)
{
    buf[0] = static_cast<uint8_t>(CTRL_CHAR::STX);
    memcpy(buf + 1, data, len);
    len++;
    uint16_t crc = Crc::Crc16_8005(buf, len);
    Cnvt::ParseU16ToAsc(crc, (char *)buf + len);
    len += 4;
    *(buf + len) = static_cast<uint8_t>(CTRL_CHAR::ETX);
    len++;
    qltdSlave->Push(groupId+'0', buf, len, 0);
    return lowerLayer->Tx(buf, len);
}

void LayerSlv::ClrRx()
{
    upperLayer->ClrRx();
}
void LayerSlv::ClrTx()
{
    lowerLayer->ClrTx();
}
