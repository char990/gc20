#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include <layer/LayerWeb.h>
#include <layer/StatusLed.h>

LayerWeb::LayerWeb(std::string name_)
{
    name = name_ + ":" + "WEB";
}

LayerWeb::~LayerWeb()
{
}

int LayerWeb::Rx(uint8_t * data, int len)
{
    StatusLed::Instance().ReloadDataSt();
    uint8_t buf[65536];
    int n = 0;
    upperLayer->Rx(buf,n);
    (void)data;
    (void)len;
/*    
    while(1)
    {
        int k = read(fd,buf,65536);
        if(k<=0)break;
        n+=k;
    }
*/    return n;
}

int LayerWeb::Tx(uint8_t * data, int len)
{
    StatusLed::Instance().ReloadDataSt();
    return lowerLayer->Tx(data, len);
}

void LayerWeb::Clean()
{
    upperLayer->Clean();
}
