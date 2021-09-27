#include <tsisp003/TsiSp003AppVer50.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Scheduler.h>

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
    case MI::CODE::SignSetHighResolutionGraphicsFrame:
        SignSetHighResolutionGraphicsFrame(data, len);
        break;
    case MI::CODE::SignConfigurationRequest:
        SignConfigurationRequest(data, len);
        break;
    case MI::CODE::SignDisplayAtomicFrames:
        SignDisplayAtomicFrames(data, len);
        break;
    default:
        if (TsiSp003AppVer31::Rx(data, len) == -1)
        {
            // if there is a new version TsiSp003, return -1
            //return -1;
            Reject(APP::ERROR::UnknownMi);
            return 0;
        }
    }
    return 0;
}

void TsiSp003AppVer50::SignSetHighResolutionGraphicsFrame(uint8_t *data, int len)
{
    if (CheckOlineReject())
    {
        SignSetFrame(data, len);
    }
}

void TsiSp003AppVer50::SignConfigurationRequest(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 1))
    {
        return;
    }
    //reply
}

// todo: not tested
void TsiSp003AppVer50::SignDisplayAtomicFrames(uint8_t *data, int len)
{
    Reject(APP::ERROR::MiNotSupported);
    return;
    if (!CheckOlineReject() || !ChkLen(len, data[2] * 2 + 3))
    {
        return;
    }
    if (data[2] != 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    auto r = sch.CmdDispAtomicFrm(data, len);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}
