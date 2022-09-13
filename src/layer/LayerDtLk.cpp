#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include <layer/LayerDtLk.h>
#include <module/MyDbg.h>
#include <module/QueueLtd.h>

QueueLtd *qltdTmc;

LayerDtLk::LayerDtLk(std::string name_, int size) : maxPktSize(size)
{
    name = name_ + ":LayerDtLk";
    buf = new uint8_t[size];
    qltdTmc = new QueueLtd(20);
}

LayerDtLk::~LayerDtLk()
{
    delete[] buf;
    delete qltdTmc;
}

int LayerDtLk::Rx(uint8_t *data, int len)
{
    uint8_t *p = data;
    for (int i = 0; i < len; i++)
    {
        uint8_t c = *p++;
        if (c == static_cast<uint8_t>(CTRL_CHAR::SOH))
        { // packet start, clear buffer
            buf[0] = c;
            length = 1;
            // Pdebug("SOH");
        }
        else if (c == static_cast<uint8_t>(CTRL_CHAR::NAK))
        {
            // Pdebug("NAK");
        } // TODO : CTRL_CHAR::NAK
        else
        {
            if (length > 0)
            {
                if (length < maxPktSize)
                {
                    buf[length++] = c;
                    if (c == static_cast<uint8_t>(CTRL_CHAR::ETX))
                    {
                        // Pdebug("ETX");
                        upperLayer->Rx(buf, length);
                        qltdTmc->Push(' ', buf, length, 0);
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

bool LayerDtLk::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerDtLk::Tx(uint8_t *data, int len)
{
    qltdTmc->Push(' ', data, len, 1);
    return lowerLayer->Tx(data, len);
}

void LayerDtLk::ClrRx()
{
    length = 0;
    upperLayer->ClrRx();
}

void LayerDtLk::ClrTx()
{
    lowerLayer->ClrTx();
}
