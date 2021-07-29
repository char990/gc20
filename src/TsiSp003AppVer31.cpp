#include <vector>
#include "TsiSp003AppVer31.h"
#include "BootTimer.h"
#include "DbHelper.h"
#include "TsiSp003Const.h"

TsiSp003AppVer31::TsiSp003AppVer31(bool & online)
:TsiSp003AppVer10(online)
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
    case MI_CODE::SignExtendedStatusRequest:
        SignExtendedStatusRequest(data, len);
        break;
    case MI_CODE::SignSetGraphicsFrame:
        SignSetGraphicsFrame(data, len);
        break;
    case MI_CODE::SignRequestStoredFMP:
        SignRequestStoredFMP(data, len);
        break;
    default:
        return TsiSp003AppVer10::Rx(data,len);
    }
    return 0;
}

void TsiSp003AppVer31::SignExtendedStatusRequest(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer31::SignSetGraphicsFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer31::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

