#include "TsiSp003AppVer10.h"

TsiSp003AppVer10::TsiSp003AppVer10()
{

}
TsiSp003AppVer10::~TsiSp003AppVer10()
{
    
}

int TsiSp003AppVer10::Tx(uint8_t * data, int len)
{
    return lowerLayer->Tx(data, len);
}

int TsiSp003AppVer10::Rx(uint8_t * data, int len)
{
    /* run app layer functions */
    //printf("TsiSp003AppVer10::Rx\n");
    return -1;
}

void TsiSp003AppVer10::PeriodicRun()
{
    
}

void TsiSp003AppVer10::Clean()
{
    
}

void TsiSp003AppVer10::Release()
{
    
}
