#include <vector>
#include "TsiSp003AppVer31.h"
#include "BootTimer.h"

TsiSp003AppVer31::TsiSp003AppVer31()
{

}
TsiSp003AppVer31::~TsiSp003AppVer31()
{
    
}

std::string TsiSp003AppVer31::Version()
{
    return std::string("Ver3.1");
}

int TsiSp003AppVer31::Received(uint8_t * data, int len)
{
    int r = TsiSp003App::Received(data,len);
    if(r==0) return 0;
    printf("Ver31::Received\n");
    return -1;
}
