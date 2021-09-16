#include <cstring>

#include <tsisp003/TsiSp003AppVer31.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Scheduler.h>
#include <module/Utils.h>

using namespace Utils;

TsiSp003AppVer31::TsiSp003AppVer31()
{

}
TsiSp003AppVer31::~TsiSp003AppVer31()
{
    
}

int TsiSp003AppVer31::Rx(uint8_t * data, int len)
{
    micode = *data;
    switch (micode)
    {
    default:
        return TsiSp003AppVer21::Rx(data,len);
    }
    return 0;
}
