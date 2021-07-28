#include <stdexcept>

#include "LayerPhcs.h"

LayerPhcs::LayerPhcs(std::string name, bool & online)
:online(online)
{
    this->name = name+":LayerPhcs";
    sessionTimeout.Setms(-1);
    displayTimeout.Setms(-1);
}

LayerPhcs::~LayerPhcs()
{
}


int LayerPhcs::Rx(uint8_t * data, int len)
{
    uint8_t buf[1024];
    int n = 0;
    rcvd+=len;
    int r = upperLayer->Rx(data, len);
    if(r==0)
    {

        sessionTimeout.Setms(30000);
    }
    else
    {

    }
    return 0;
}

int LayerPhcs::Tx(uint8_t * data, int len)
{
    
    
    
    
    
    
    
    
    return lowerLayer->Tx(data,len);
}

void LayerPhcs::PeriodicRun()
{
    SessionTimeout();
    DisplayTimeout();
}

void LayerPhcs::Clean()
{
    nr=0;
    ns=0;
    sessionTimeout.Setms(-1);
    displayTimeout.Setms(-1);
}

void LayerPhcs::Release()
{
    lowerLayer->Release();
}


/// -------------------------------------------------------
void LayerPhcs::SessionTimeout()
{
    if(sessionTimeout.IsExpired())
    {
        printf("[%s]Session timeout=%dbytes\n", name.c_str(),rcvd);
        sessionTimeout.Setms(-1);
        rcvd=0;
    }
}

void LayerPhcs::DisplayTimeout()
{
    if(displayTimeout.IsExpired())
    {
        printf("[%s]Display timeout\n", name.c_str());
        displayTimeout.Setms(-1);
    }
}

void LayerPhcs::IncNr()
{
    nr++;
    if(nr==0)nr=1;
}

void LayerPhcs::IncNs()
{
    ns++;
    if(ns==0)ns=1;
}
