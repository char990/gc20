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
    auto mi = static_cast<MI::CODE>(micode);
    switch (mi)
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
    if (CheckOnline_RejectIfFalse())
    {
        SignSetFrame(data, len);
    }
}

void TsiSp003AppVer50::SignConfigurationRequest(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1))
    {
        return;
    }
    uint8_t *p = txbuf;
    *p++ = static_cast<uint8_t>(MI::CODE::SignConfigurationReply);
    memcpy(p, prod.MfcCode(), 10);
    p += 10;
    auto &groups = Controller::Instance().GetGroups();
    *p++ = groups.size();
    for (auto &g : groups)
    {
        *p++ = g->GroupId();
        auto &signs = g->GetSigns();
        *p++ = signs.size();
        for (auto &s : signs)
        {
            p = s->GetConfig(p);
        }
        *p++ = 0; // signature data bytes
    }
    Tx(txbuf, p - txbuf);
}

// todo: not tested
void TsiSp003AppVer50::SignDisplayAtomicFrames(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, data[2] * 2 + 3))
    {
        return;
    }
    if (data[2] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    auto r = ctrller.CmdDispAtomicFrm(data, len);
    if (r == APP::ERROR::AppNoError)
    {
        Ack();
    }
    else
    {
        Reject(r);
    }
}
