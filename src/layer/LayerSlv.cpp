#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <memory>

#include <layer/LayerSlv.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/QueueLtd.h>

QueueLtd *qltdSlave;

using namespace Utils;

LayerSlv::LayerSlv(std::string name_, int groupId, int maxPktSize)
    : length(0), maxPktSize(maxPktSize), rxbuf(maxPktSize)
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
    //    rxbuf.Push(data, len);
    // TODO PrintRxBuf may crush, disable rxbuf
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
                    if (c == static_cast<uint8_t>(CTRL_CHAR::ETX) || isxdigit(c))
                    {
                        buf[length++] = c;
                        if (c == static_cast<uint8_t>(CTRL_CHAR::ETX))
                        {
                            if (length >= 34 && (length & 1) == 0)
                            {
                                uint16_t crc1 = Crc::Crc16_8005(buf, length - 5);
                                uint16_t crc2 = Cnvt::ParseToU16((char *)buf + length - 5);
                                if (crc1 == crc2)
                                {
                                    upperLayer->Rx(buf + 1, length - 6);
                                    rxbuf.Reset();
                                }
                                else
                                {
                                    DebugPrt("LayerSlv Rx CRC error: %04X!=%04X", crc1, crc2);
                                }
                                qltdSlave->PushBack(groupId + '0', buf, length, 1);
                            }
                            length = 0;
                            return 0; // only deal with one pkt. Discard other data.
                        }
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
    qltdSlave->PushBack(groupId + '0', buf, len, 0);
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

// TODO PrintRxBuf may crush
void LayerSlv::PrintRxBuf()
{
    int cnt = rxbuf.Cnt();
    DebugPrt("SLV rxbuf(size=%d):", cnt);
    if (cnt > 0)
    {
        std::unique_ptr<uint8_t> p(new uint8_t(cnt));
        uint8_t *b = p.get();
        auto x = rxbuf.Pop(b, cnt);
        for (int i = 0; i < x; i++)
        {
            auto c = *b;
            PrintAsc(c);
            if (c == static_cast<uint8_t>(CTRL_CHAR::ETX))
            {
                printf("\n");
            }
            b++;
        }
        printf("\n");
    }
    rxbuf.Reset();
}
