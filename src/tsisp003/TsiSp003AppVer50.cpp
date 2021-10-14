#include <tsisp003/TsiSp003AppVer50.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Controller.h>

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
    auto r = ctrller.CmdDispAtomicFrm(data, len);
    if (r == APP::ERROR::AppNoError)
    {
        char buf[64];
        int len = sprintf(buf, "SignDisplayAtomicFrames: GroupId=%d", data[1]);
        uint8_t *p=data+3;
        for(int i=0;i<data[2];i++)
        {
            len+=snprintf(buf+len, 63-len ,":%d,%d", p[0], p[1]);
            p+=2;
        }
        db.GetUciEvent().Push(0, buf);
        Ack();
    }
    else
    {
        Reject(r);
    }
}
