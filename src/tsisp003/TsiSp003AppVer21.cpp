#include <ctime>
#include <cstring>
#include <tsisp003/TsiSp003AppVer21.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Controller.h>
#include <module/Utils.h>

using namespace Utils;
using namespace std;
extern time_t GetTime(time_t *);

TsiSp003AppVer21::TsiSp003AppVer21()
{
}
TsiSp003AppVer21::~TsiSp003AppVer21()
{
}

uint8_t * TsiSp003AppVer21::Time2Buf(uint8_t *p)
{
    struct tm t;
    time_t t_ = GetTime(nullptr);
    localtime_r(&t_, &t);
    *p++ = t.tm_mday;
    *p++ = t.tm_mon + 1;
    int year = t.tm_year + 1900;
    *p++ = year >> 8;
    *p++ = year & 0xFF;
    *p++ = t.tm_hour;
    *p++ = t.tm_min;
    *p++ = t.tm_sec;
    return p;
}

int TsiSp003AppVer21::Rx(uint8_t *data, int len)
{
    micode = *data;
    auto mi = static_cast<MI::CODE>(micode);
    switch (mi)
    {
    case MI::CODE::HeartbeatPoll:
        HeartbeatPoll(data, len);
        break;
    case MI::CODE::UpdateTime:
        UpdateTime(data, len);
        break;
    case MI::CODE::SystemReset:
        SystemReset(data, len);
        break;
    case MI::CODE::SignSetTextFrame:
    case MI::CODE::SignSetGraphicsFrame:
        SignSetFrame(data, len);
        break;
    case MI::CODE::SignSetMessage:
        SignSetMessage(data, len);
        break;
    case MI::CODE::SignSetPlan:
        SignSetPlan(data, len);
        break;
    case MI::CODE::SignDisplayFrame:
        SignDisplayFrame(data, len);
        break;
    case MI::CODE::SignDisplayMessage:
        SignDisplayMessage(data, len);
        break;
    case MI::CODE::EnablePlan:
    case MI::CODE::DisablePlan:
        EnDisPlan(data, len);
        break;
    case MI::CODE::RequestEnabledPlans:
        RequestEnabledPlans(data, len);
        break;
    case MI::CODE::SignSetDimmingLevel:
        SignSetDimmingLevel(data, len);
        break;
    case MI::CODE::PowerOnOff:
        PowerOnOff(data, len);
        break;
    case MI::CODE::DisableEnableDevice:
        DisableEnableDevice(data, len);
        break;
    case MI::CODE::SignRequestStoredFMP:
        SignRequestStoredFMP(data, len);
        break;
    case MI::CODE::RetrieveFaultLog:
        RetrieveFaultLog(data, len);
        break;
    case MI::CODE::ResetFaultLog:
        ResetFaultLog(data, len);
        break;
    case MI::CODE::SignExtendedStatusRequest:
        SignExtendedStatusRequest(data, len);
        break;
    default:
        return TsiSp003App::Rx(data, len);
    }
    return 0;
}

void TsiSp003AppVer21::HeartbeatPoll(uint8_t *data, int len)
{
    if (ChkLen(len, 1) == false)
        return;
    SignStatusReply();
}

void TsiSp003AppVer21::SignStatusReply()
{
    uint8_t * p = txbuf;
    *p++ = static_cast<uint8_t>(MI::CODE::SignStatusReply);
    *p++ = IsOnline() ? 1 : 0;
    *p++ = static_cast<uint8_t>(appErr);
    p = Time2Buf(p);
    p = Cnvt::PutU16(db.HdrChksum(),p);
    *p++ = static_cast<uint8_t>(ctrller.ctrllerError.GetErrorCode());
    int scnt = ucihw.NumberOfSigns();
    *p++ = scnt;
    for (int j = 1; j <= scnt; j++)
    {
        for (int i = 1; i <= ctrller.GroupCnt(); i++)
        {
            auto sign = ctrller.GetGroup(i)->GetSign(j);
            if (sign != nullptr)
            {
                p = sign->GetStatus(p);
                break;
            }
        }
    }
    Tx(txbuf, p - txbuf);
}

void TsiSp003AppVer21::UpdateTime(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 8))
        return;
    // set time
    struct tm stm;
    data++;
    stm.tm_mday = *data++;
    stm.tm_mon = *data - 1;
    data++;
    stm.tm_year = Cnvt::GetU16(data) - 1900;
    data += 2;
    stm.tm_hour = *data++;
    stm.tm_min = *data++;
    stm.tm_sec = *data;
    stm.tm_isdst = -1;
    if (ctrller.CmdUpdateTime(stm) == APP::ERROR::AppNoError)
    {
        Ack();
    }
    else
    {
        SetRejectStr("Invalid time");
        Reject(APP::ERROR::SyntaxError);
    }
}

void TsiSp003AppVer21::SystemReset(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = ctrller.CmdSystemReset(data, rejectStr);
    (r == APP::ERROR::AppNoError) ? Ack(): Reject(r);
}

void TsiSp003AppVer21::SignSetFrame(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
    {
        return;
    }
    if (data[1] == 0)
    {
        sprintf(rejectStr,"SignSetFrame Error:FrameID=0");
        Reject(APP::ERROR::SyntaxError);
        return;
    }

    auto r = ctrller.SignSetFrame(data, len, rejectStr);
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::SignDisplayFrame(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = ctrller.CmdDispFrm(data);
    if (r == APP::ERROR::AppNoError)
    {
        Ack();
    }
    else
    {
        SetRejectStr("Group[%d]SignDisplayFrame:[%d]",data[1],data[2]);
        Reject(r);
    }
}

void TsiSp003AppVer21::SignSetMessage(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
    {
        return;
    }
    auto r = ctrller.SignSetMessage(data, len, rejectStr);
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::SignDisplayMessage(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = ctrller.CmdDispMsg(data);
    if (r == APP::ERROR::AppNoError)
    {
        Ack();
    }
    else
    {
        SetRejectStr("Group[%d]SignDisplayMessage:[%d]",data[1],data[2]);
        Reject(r);
    }
}

void TsiSp003AppVer21::SignSetPlan(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
    {
        return;
    }
    auto r = ctrller.SignSetPlan(data, len, rejectStr);
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::EnDisPlan(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = ctrller.CmdEnDisPlan(data);
    const char * endis = data[0] == static_cast<uint8_t>(MI::CODE::EnablePlan)?"En":"Dis";
    if (r == APP::ERROR::AppNoError)
    {
        Ack();
    }
    else
    {
        SetRejectStr("Group[%d]%sable Plan[%d]", data[1], endis, data[2]);
        Reject(r);
    }
}

void TsiSp003AppVer21::RequestEnabledPlans(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1))
    {
        return;
    }
    int bytes = ctrller.CmdRequestEnabledPlans(txbuf);
    Tx(txbuf, bytes);
}

void TsiSp003AppVer21::SignSetDimmingLevel(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 2 + data[1] * 3))
    {
        return;
    }
    auto r = ctrller.CmdSetDimmingLevel(data, rejectStr);
    (r == APP::ERROR::AppNoError) ? Ack(): Reject(r);
}

void TsiSp003AppVer21::PowerOnOff(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 2 + data[1] * 2))
    {
        return;
    }
    auto r = ctrller.CmdPowerOnOff(data, rejectStr);
    (r == APP::ERROR::AppNoError) ? Ack(): Reject(r);
}

void TsiSp003AppVer21::DisableEnableDevice(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 2 + data[1] * 2))
    {
        return;
    }
    auto r = ctrller.CmdDisableEnableDevice(data, rejectStr);
    (r == APP::ERROR::AppNoError) ? Ack(): Reject(r);
}

void TsiSp003AppVer21::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    if (data[1] > 3)
    {
        SetRejectStr("Unknow type:%d",data[1]);
        Reject(APP::ERROR::SyntaxError);
    }
    if (data[2] == 0)
    {
        SetRejectStr("Frame/Message/Plan[0] is not valid");
        Reject(APP::ERROR::SyntaxError);
    }
    switch (data[1])
    {
    case 0: // frm
    {
        auto frm = db.GetUciFrm().GetStFrm(data[2]);
        if (frm == nullptr)
        {
            SetRejectStr("Frame[%d] undefined",data[2]);
            Reject(APP::ERROR::FrmMsgPlnUndefined);
        }
        else
        {
            Tx(frm->rawData.data(), frm->rawData.size());
        }
    }
    break;
    case 1: // msg
    {
        auto msg = db.GetUciMsg().GetMsg(data[2]);
        if (msg == nullptr)
        {
            SetRejectStr("Msg[%d] undefined",data[2]);
            Reject(APP::ERROR::FrmMsgPlnUndefined);
        }
        else
        {
            uint8_t a[MSG_LEN_MAX + MSG_TAIL];
            int len = msg->ToArray(a);
            Tx(a, len-2);   // do not include crc
        }
    }
    break;
    case 2: // pln
    {
        auto pln = db.GetUciPln().GetPln(data[2]);
        if (pln == nullptr)
        {
            SetRejectStr("Plan[%d] undefined",data[2]);
            Reject(APP::ERROR::FrmMsgPlnUndefined);
        }
        else
        {
            uint8_t a[PLN_LEN_MAX + PLN_TAIL];
            int len = pln->ToArray(a);
            Tx(a, len-2);   // do not include crc
        }
    }
    break;
    }
}

void TsiSp003AppVer21::SignExtendedStatusRequest(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1))
    {
        return;
    }
    uint8_t * p=txbuf;
    *p++ = static_cast<uint8_t>(MI::CODE::SignExtendedStatusReply);
    *p++ = IsOnline() ? 1 : 0;
    *p++ = static_cast<uint8_t>(appErr);
    memcpy(p, ucihw.MfcCode(), 10); p+=10;
    p=Time2Buf(p);
    *p++ = static_cast<uint8_t>(ctrller.ctrllerError.GetErrorCode());
    int scnt = ucihw.NumberOfSigns();
    *p++ = scnt;
    for (int j = 1; j <= scnt; j++)
    {
        for (int i = 1; i <= ctrller.GroupCnt(); i++)
        {
            auto sign = ctrller.GetGroup(i)->GetSign(j);
            if (sign != nullptr)
            {
                p = sign->GetExtStatus(p);
                break;
            }
        }
    }
    int applen = p - txbuf;
    uint16_t crc = Crc::Crc16_1021(txbuf, applen);
    txbuf[applen++] = crc >> 8;
    txbuf[applen++] = crc;
    Tx(txbuf, applen);
}

void TsiSp003AppVer21::RetrieveFaultLog(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1))
    {
        return;
    }
    int applen = db.GetUciFault().GetFaultLog20(txbuf + 1);
    txbuf[0] = static_cast<uint8_t>(MI::CODE::FaultLogReply);
    Tx(txbuf, applen + 1);
}

void TsiSp003AppVer21::ResetFaultLog(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1))
    {
        return;
    }
    if(ucihw.IsResetLogAllowed())
    {
        db.GetUciFault().Reset();
        db.GetUciEvent().Push(0, "ResetFaultLog");
        Ack();
    }
    else
    {
        Reject(APP::ERROR::MiNotSupported);
    }
}
