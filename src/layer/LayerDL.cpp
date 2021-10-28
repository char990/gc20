#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include <layer/LayerDL.h>

LayerDL::LayerDL(std::string name_, int size):maxPktSize(size)
{
    name = name_ + ":" + "DL";
    buf = new uint8_t[size];
}

LayerDL::~LayerDL()
{
    delete [] buf;
}

int LayerDL::Rx(uint8_t *data, int len)
{
    uint8_t *p = data;
    for (int i = 0; i < len; i++)
    {
        uint8_t c = *p++;
        if (c == static_cast<uint8_t>(CTRL_CHAR::SOH))
        {// packet start, clear buffer
            buf[0] = c;
            length = 1;
        }
        else if(c == static_cast<uint8_t>(CTRL_CHAR::NAK))
        {}  // todo:
        else
        {
            if (length > 0)
            {
                if (length < maxPktSize)
                {
                    buf[length++] = c;
                    if (c == static_cast<uint8_t>(CTRL_CHAR::ETX))
                    {
                        upperLayer->Rx(buf, length);
                        length = 0;
                        return 0;   // only deal with one pkt. Discard other data. 
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


bool LayerDL::IsTxReady()
{
    return lowerLayer->IsTxReady();
}

int LayerDL::Tx(uint8_t *data, int len)
{
    return lowerLayer->Tx(data, len);
}

void LayerDL::ClrRx()
{
    length = 0;
    upperLayer->ClrRx();
}

void LayerDL::ClrTx()
{
    lowerLayer->ClrTx();
}
