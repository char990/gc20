#include <vector>
#include "TsiSp003AppVer50.h"
#include "BootTimer.h"

TsiSp003AppVer50::TsiSp003AppVer50()
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
    printf("Ver50::Received\n");
    return -1;
}
