#include "TsiSp003AppVer10.h"

TsiSp003AppVer10::TsiSp003AppVer10()
{

}
TsiSp003AppVer10::~TsiSp003AppVer10()
{
    
}

int TsiSp003AppVer10::Rx(uint8_t * data, int len)
{
    if(NewMi(data, len)==0)
    {
        return 0;
    }
    /* run app layer functions */
    return -1;
}

int TsiSp003AppVer10::NewMi(uint8_t * data, int len)
{
    return -1;
}
