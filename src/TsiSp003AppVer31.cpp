#include <vector>
#include "TsiSp003AppVer31.h"
#include "BootTimer.h"

TsiSp003AppVer31::TsiSp003AppVer31()
{

}
TsiSp003AppVer31::~TsiSp003AppVer31()
{
    
}

int TsiSp003AppVer31::Rx(uint8_t * data, int len)
{
    if(NewMi(data, len)==0)
    {
        return 0;
    }
    int r = TsiSp003AppVer10::Rx(data,len);
    if(r==0) return 0;
    //printf("TsiSp003AppVer31::Rx\n");
    return -1;
}

int TsiSp003AppVer31::NewMi(uint8_t * data, int len)
{

    return -1;
}
