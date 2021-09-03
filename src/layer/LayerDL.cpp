#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include <layer/LayerDL.h>

LayerDL::LayerDL(std::string name_, int size):size(size)
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
        if (c == DATALINK::CTRL_CHAR::SOH)
        {// packet start, clear buffer
            buf[0] = c;
            length = 1;
        }
        else if(c == DATALINK::CTRL_CHAR::NAK)
        {}  // todo:
        else
        {
            if (length > 0)
            {
                if (length < size - 1)
                {
                    buf[length++] = c;
                    if (c == DATALINK::CTRL_CHAR::ETX)
                    {
                        upperLayer->Rx(buf, length);
                        length = 0;
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

int LayerDL::Tx(uint8_t *data, int len)
{
    return lowerLayer->Tx(data, len);
}

void LayerDL::Clean()
{
    length = 0;
    upperLayer->Clean();
}
