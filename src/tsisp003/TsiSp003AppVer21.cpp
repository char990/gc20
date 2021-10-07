#include <ctime>
#include <cstring>
#include <tsisp003/TsiSp003AppVer21.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Scheduler.h>
#include <module/Utils.h>

using namespace Utils;
using namespace std;

TsiSp003AppVer21::TsiSp003AppVer21()
{
}
TsiSp003AppVer21::~TsiSp003AppVer21()
{
}

void TsiSp003AppVer21::Time2Buf(uint8_t *p)
{
    struct tm t;
    time_t t_ = time(nullptr);
    localtime_r(&t_, &t);
    *p++ = t.tm_mday;
    *p++ = t.tm_mon + 1;
    int year = t.tm_year + 1900;
    *p++ = year >> 8;
    *p++ = year & 0xFF;
    *p++ = t.tm_hour;
    *p++ = t.tm_min;
    *p++ = t.tm_sec;
}

int TsiSp003AppVer21::Rx(uint8_t *data, int len)
{
    micode = *data;
    switch (micode)
    {
    case MI::CODE::HeartbeatPoll:
        HeartbeatPoll(data, len);
        break;
    case MI::CODE::SignSetTextFrame:
        SignSetTextFrame(data, len);
        break;
    case MI::CODE::SignSetGraphicsFrame:
        SignSetGraphicsFrame(data, len);
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
        EnablePlan(data, len);
        break;
    case MI::CODE::DisablePlan:
        DisablePlan(data, len);
        break;
    case MI::CODE::RequestEnabledPlans:
        RequestEnabledPlans(data, len);
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
    txbuf[0] = MI::CODE::SignStatusReply;
    txbuf[1] = IsOnline() ? 1 : 0;
    txbuf[2] = appErr;
    Time2Buf(txbuf + 3);
    uint16_t i16 = db.HdrChksum();
    txbuf[10] = i16 >> 8;
    txbuf[11] = i16 & 0xff;
    txbuf[12] = sch.CtrllerErr();
    int scnt = db.GetUciProd().NumberOfSigns();
    txbuf[13] = scnt;
    uint8_t *p = &txbuf[14];
    for (int j = 1; j <= scnt; j++)
    {
        for (int i = 1; i <= sch.GroupCnt(); i++)
        {
            auto sign = sch.GetGroup(i)->GetSign(j);
            if (sign != nullptr)
            {
                p = sign->GetStatus(p);
                break;
            }
        }
    }
    Tx(txbuf, p - txbuf);
}

void TsiSp003AppVer21::SignSetFrame(uint8_t *data, int len)
{
    auto r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_FRM_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (sch.IsFrmActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else if (id <= db.GetUciUser().LockedFrm()) // && pstatus->rFSstate() != Status::FS_OFF)
    {
        r = APP::ERROR::OverlaysNotSupported; // pre-defined and can't be overlapped
    }
    else
    {
        UciFrm &frm = db.GetUciFrm();
        r = frm.SetFrm(data, len);
        if (r == APP::ERROR::AppNoError)
        {
            frm.SaveFrm(id);
        }
    }
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::SignSetTextFrame(uint8_t *data, int len)
{
    if (!CheckOlineReject())
    {
        return;
    }
    SignSetFrame(data, len);
}

void TsiSp003AppVer21::SignSetGraphicsFrame(uint8_t *data, int len)
{
    if (!CheckOlineReject())
    {
        return;
    }
    SignSetFrame(data, len);
}

void TsiSp003AppVer21::SignDisplayFrame(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = sch.CmdDispFrm(data);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::SignSetMessage(uint8_t *data, int len)
{
    if (!CheckOlineReject())
    {
        return;
    }
    auto r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_MSG_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (len > MSG_LEN_MAX)
    {
        r = APP::ERROR::LengthError;
    }
    else if (sch.IsMsgActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else if (id <= db.GetUciUser().LockedMsg()) // && pstatus->rFSstate() != Status::FS_OFF)
    {
        r = APP::ERROR::OverlaysNotSupported; // pre-defined and can't be overlapped
    }
    else
    {
        Cnvt::PutU16(Crc::Crc16_1021(data, len), data + len); // attach CRC
        UciMsg &msg = db.GetUciMsg();
        r = msg.SetMsg(data, len + 2);
        if (r == APP::ERROR::AppNoError)
        {
            msg.SaveMsg(id);
        }
    }
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::SignDisplayMessage(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = sch.CmdDispMsg(data);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::SignSetPlan(uint8_t *data, int len)
{
    if (!CheckOlineReject())
    {
        return;
    }
    auto r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_PLN_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (len > PLN_LEN_MAX)
    {
        r = APP::ERROR::LengthError;
    }
    else if (sch.IsPlnActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else
    {
        Cnvt::PutU16(Crc::Crc16_1021(data, len), data + len); // attach CRC
        UciPln &pln = db.GetUciPln();
        r = pln.SetPln(data, len + PLN_TAIL);
        if (r == APP::ERROR::AppNoError)
        {
            pln.SavePln(id);
        }
    }
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::EnablePlan(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = sch.CmdEnablePlan(data);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::DisablePlan(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = sch.CmdDisablePlan(data);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::RequestEnabledPlans(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 1))
    {
        return;
    }
    int bytes = sch.CmdRequestEnabledPlans(txbuf);
    Tx(txbuf, bytes);
}

void TsiSp003AppVer21::SignSetDimmingLevel(uint8_t *data, int len)
{
    if (data[1] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    else if (!CheckOlineReject() || !ChkLen(len, 2 + data[1] * 3))
    {
        return;
    }
    auto r = sch.CmdSetDimmingLevel(data);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::PowerOnOff(uint8_t *data, int len)
{
    if (data[1] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    else if (!CheckOlineReject() || !ChkLen(len, 2 + data[1] * 2))
    {
        return;
    }
    auto r = sch.CmdPowerOnOff(data, len);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::DisableEnableDevice(uint8_t *data, int len)
{
    if (data[1] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    else if (!CheckOlineReject() || !ChkLen(len, 2 + data[1] * 2))
    {
        return;
    }
    auto r = sch.CmdDisableEnableDevice(data, len);
    (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
}

void TsiSp003AppVer21::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 3))
    {
        return;
    }
    if (data[1] > 3 || data[2] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    switch (data[1])
    {
    case 0: // frm
    {
        auto frm = db.GetUciFrm().GetStFrm(data[2]);
        if (frm == nullptr)
        {
            Reject(APP::ERROR::FrmMsgPlnUndefined);
        }
        else
        {
            Tx(frm->rawData, frm->dataLen);
        }
    }
    break;
    case 1: // msg
    {
        auto msg = db.GetUciMsg().GetMsg(data[2]);
        if (msg == nullptr)
        {
            Reject(APP::ERROR::FrmMsgPlnUndefined);
        }
        else
        {
            uint8_t a[MSG_LEN_MAX + MSG_TAIL];
            int len = msg->ToArray(a);
            Tx(a, len);
        }
    }
    break;
    case 2: // pln
    {
        auto pln = db.GetUciPln().GetPln(data[2]);
        if (pln == nullptr)
        {
            Reject(APP::ERROR::FrmMsgPlnUndefined);
        }
        else
        {
            uint8_t a[PLN_LEN_MAX + PLN_TAIL];
            int len = pln->ToArray(a);
            Tx(a, len);
        }
    }
    break;
    }
}

void TsiSp003AppVer21::SignExtendedStatusRequest(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 1))
    {
        return;
    }
    txbuf[0] = MI::CODE::SignExtendedStatusReply;
    txbuf[1] = IsOnline() ? 1 : 0;
    txbuf[2] = appErr;
    UciProd &prod = db.GetUciProd();
    memcpy(txbuf + 3, prod.MfcCode(), 10);
    Time2Buf(txbuf + 13);
    txbuf[20] = sch.CtrllerErr();
    int scnt = prod.NumberOfSigns();
    txbuf[21] = scnt;
    uint8_t *p = &txbuf[22];
    for (int j = 1; j <= scnt; j++)
    {
        for (int i = 1; i <= sch.GroupCnt(); i++)
        {
            auto sign = sch.GetGroup(i)->GetSign(j);
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
    if (!CheckOlineReject() || !ChkLen(len, 1))
    {
        return;
    }
    //Tx(txbuf, applen);
}

void TsiSp003AppVer21::ResetFaultLog(uint8_t *data, int len)
{
    if (!CheckOlineReject() || !ChkLen(len, 1))
    {
        return;
    }
    // reset fault log
    Ack();
}
