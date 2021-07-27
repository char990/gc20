#include "TsiSp003Prst.h"

int TsiSp003Prst::Rx(uint8_t * data, int len)
{
    // translate
    
    
    return upperLayer->Rx(data,len);
}

int TsiSp003Prst::Tx(uint8_t * data, int len)
{
    // translate
   
    
    
    
    return  lowerLayer->Tx(data,len);
}

void TsiSp003Prst::PeriodicRun()
{
    upperLayer->PeriodicRun();
}

void TsiSp003Prst::Clean()
{
    upperLayer->Clean();
}

void TsiSp003Prst::Release()
{
    lowerLayer->Release();
}
