#include <vector>
#include "TsiSp003App.h"
#include "BootTimer.h"

TsiSp003App::TsiSp003App()
{

}
TsiSp003App::~TsiSp003App()
{
    
}


int TsiSp003App::Rx(uint8_t * data, int len)
{
    return -1;
}

int TsiSp003App::Tx(uint8_t * data, int len)
{
    return lowerlayer->Tx(data, len);
}

