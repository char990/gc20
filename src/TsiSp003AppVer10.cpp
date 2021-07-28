#include "TsiSp003AppVer10.h"

TsiSp003AppVer10::TsiSp003AppVer10(bool & online)
:TsiSp003App(online)
{

}
TsiSp003AppVer10::~TsiSp003AppVer10()
{
    
}

int TsiSp003AppVer10::Rx(uint8_t * data, int len)
{
    printf("TsiSp003AppVer10::Rx\n");
    return -1;
}



