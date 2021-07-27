#include "TsiSp003App.h"

TsiSp003App::TsiSp003App()
{
    if(0)
    {
        iAppLayer = new TsiSp003AppVer31();
    }
    else    //    if(50) default
    {
        iAppLayer = new TsiSp003AppVer50();
    }
}

TsiSp003App::~TsiSp003App()
{
    delete iAppLayer;
}

int TsiSp003App::Rx(uint8_t * data, int len)
{
    return  iAppLayer->Rx(data,len);
}

int TsiSp003App::Tx(uint8_t * data, int len)
{
    return  iAppLayer->Tx(data,len);
}

void TsiSp003App::PeriodicRun()
{
    iAppLayer->PeriodicRun();
}

void TsiSp003App::Clean()
{
    iAppLayer->Clean();
}

void TsiSp003App::Release()
{
    iAppLayer->Release();
}
