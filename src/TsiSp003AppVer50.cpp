#include <vector>
#include "TsiSp003AppVer50.h"
#include "BootTimer.h"

TsiSp003AppVer50::TsiSp003AppVer50()
{

}
TsiSp003AppVer50::~TsiSp003AppVer50()
{
    
}

std::string TsiSp003AppVer50::Version()
{
    return std::string("Ver5.0");
}

int TsiSp003AppVer50::Received(uint8_t * data, int len)
{
    int r = TsiSp003AppVer31::Received(data,len);
    if(r==0) return 0;
    printf("Ver50::Received\n");
    return -1;
}
