#include <vector>
#include "TsiSp003AppVer50.h"
#include "BootTimer.h"

extern void PrintTime();

TsiSp003AppVer50::TsiSp003AppVer50(bool & online)
:TsiSp003AppVer31(online)
{

}
TsiSp003AppVer50::~TsiSp003AppVer50()
{
    
}

int TsiSp003AppVer50::Rx(uint8_t * data, int len)
{
    if(NewMi(data, len)==0)
    {
        return 0;
    }
    int r = TsiSp003AppVer31::Rx(data,len);
    if(r==0) return 0;
    //printf("TsiSp003AppVer50::Rx\n");
    PrintTime();
    uint8_t buf[128*1024];
    Tx(buf, 128*1024);
    return -1;
}

int TsiSp003AppVer50::NewMi(uint8_t * data, int len)
{

    return -1;
}

