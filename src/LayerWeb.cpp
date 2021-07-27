#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <stdexcept>

#include "LayerWeb.h"

LayerWeb::LayerWeb(std::string name)
{
    this->name = name+":LayerWeb";
}

LayerWeb::~LayerWeb()
{
}

int LayerWeb::Rx(uint8_t * data, int len)
{
    uint8_t buf[65536];
    int n = 0;
    upperLayer->Rx(buf,n);
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
    return lowerLayer->Tx(data, len);
}

void LayerWeb::PeriodicRun()
{
        
}

void LayerWeb::Clean()
{
        
}

void LayerWeb::Release()
{
    lowerLayer->Release();
}
