#include "TsiSp003AppVer50.h"
#include "TsiSp003Const.h"
#include "DbHelper.h"
#include "Controller.h"


extern void PrintTime();

TsiSp003AppVer50::TsiSp003AppVer50()
{
}
TsiSp003AppVer50::~TsiSp003AppVer50()
{
}

int TsiSp003AppVer50::Rx(uint8_t *data, int len)
{
    micode = *data;
    switch (micode)
    {
    case MI_CODE::SignRequestStoredFMP:
        SignRequestStoredFMP(data, len);
        break;
    case MI_CODE::SignSetHighResolutionGraphicsFrame:
        SignSetHighResolutionGraphicsFrame(data, len);
        break;
    case MI_CODE::SignConfigurationRequest:
        SignConfigurationRequest(data, len);
        break;
    case MI_CODE::SignDisplayAtomicFrames:
        SignDisplayAtomicFrames(data, len);
        break;
    default:
        if (TsiSp003AppVer31::Rx(data, len) == -1)
        {
            // if there is a new version TsiSp003, return -1
            //return -1;
            Reject(APP_ERROR::UnknownMi);
            return 0;
        }
    }
    return 0;
}

void TsiSp003AppVer50::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer50::SignSetHighResolutionGraphicsFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer50::SignConfigurationRequest(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer50::SignDisplayAtomicFrames(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}
