#include "TsiSp003App.h"
#include "TsiSp003Cfg.h"
#include "TsiSp003AppVer50.h"

TsiSp003App::TsiSp003App(bool & online)
:online(online)
{
}

TsiSp003App::~TsiSp003App()
{
}

int TsiSp003App::Tx(uint8_t * data, int len)
{
    return lowerLayer->Tx(data, len);
}

int TsiSp003App::Rx(uint8_t * data, int len)
{
    return -1;
}

void TsiSp003App::PeriodicRun()
{
    
}

void TsiSp003App::Clean()
{
    
}

void TsiSp003App::Release()
{
    
}
