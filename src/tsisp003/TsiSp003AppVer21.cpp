#include <ctime>
#include <cstring>
#include <tsisp003/TsiSp003AppVer21.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Controller.h>
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
    auto mi = static_cast<MI::CODE>(micode);
    switch (mi)
    {
    case MI::CODE::HeartbeatPoll:
        HeartbeatPoll(data, len);
        break;
    case MI::CODE::SystemReset:
        SystemReset(data, len);
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
    txbuf[0] = static_cast<uint8_t>(MI::CODE::SignStatusReply);
    txbuf[1] = IsOnline() ? 1 : 0;
    txbuf[2] = static_cast<uint8_t>(appErr);
    Time2Buf(txbuf + 3);
    uint16_t i16 = db.HdrChksum();
    txbuf[10] = i16 >> 8;
    txbuf[11] = i16 & 0xff;
    txbuf[12] = static_cast<uint8_t>(ctrller.ctrllerError.GetErrorCode());
    int scnt = db.GetUciProd().NumberOfSigns();
    txbuf[13] = scnt;
    uint8_t *p = &txbuf[14];
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

void TsiSp003AppVer21::SystemReset(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    uint8_t gid = data[1];
    uint8_t level = data[2];
    if ((level > 3 && level < 255) ||
        (level > 1 && gid != 0))
    {
        Reject(APP::ERROR::SyntaxError);
        return;
    }
    auto r = ctrller.CmdSystemReset(data);
    if (r == APP::ERROR::AppNoError)
    {
        db.GetUciEvent().Push(0, "SystemReset: Group%d, Level%d", gid, level);
        Ack();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::SignSetFrame(uint8_t *data, int len)
{
    auto r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_FRM_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (ctrller.IsFrmActive(id))
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
            switch (*data)
            {
            case static_cast<uint8_t>(MI::CODE::SignSetTextFrame):
                db.GetUciEvent().Push(0, "SignSetTextFrame: Frame%d", id);
                break;
            case static_cast<uint8_t>(MI::CODE::SignSetGraphicsFrame):
                db.GetUciEvent().Push(0, "SignSetGraphicsFrame: Frame%d", id);
                break;
            default:
                db.GetUciEvent().Push(0, "SignSetHighResolutionGraphicsFrame: Frame%d", id);
                break;
            }
        }
    }
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::SignSetTextFrame(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
    {
        return;
    }
    SignSetFrame(data, len);
}

void TsiSp003AppVer21::SignSetGraphicsFrame(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
    {
        return;
    }
    SignSetFrame(data, len);
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
        db.GetUciEvent().Push(0, "SignDisplayFrame: Group%d, Frame%d", data[1], data[2]);
        Ack();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::SignSetMessage(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
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
    else if (ctrller.IsMsgActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else if (id <= db.GetUciUser().LockedMsg()) // && pstatus->rFSstate() != Status::FS_OFF)
    {
        r = APP::ERROR::OverlaysNotSupported; // pre-defined and can't be overlapped
    }
    else
    {
        uint8_t *fid = data + 4;
        for (int i = 0; i < 6; i++) // overlay frames in msg not supported
        {
            if (*fid == 0)
            {
                break;
            }
            else
            {
                if (*(fid + 1) == 0)
                {
                    if (i < 5 && *(fid + 2) != 0)
                    {
                        r = APP::ERROR::OverlaysNotSupported;
                        break;
                    }
                }
            }
            fid += 2;
        }
        if (r == APP::ERROR::AppNoError)
        {
            Cnvt::PutU16(Crc::Crc16_1021(data, len), data + len); // attach CRC
            UciMsg &msg = db.GetUciMsg();
            r = msg.SetMsg(data, len + 2);
            if (r == APP::ERROR::AppNoError)
            {
                msg.SaveMsg(id);
                db.GetUciEvent().Push(0, "SignSetMessage: Msg%d", id);
            }
        }
    }
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
        db.GetUciEvent().Push(0, "SignDisplayMessage: Group%d, Msg%d", data[1], data[2]);
        Ack();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::SignSetPlan(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse())
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
    else if (ctrller.IsPlnActive(id))
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
            db.GetUciEvent().Push(0, "SignSetPlan: Plan%d", data[1]);
        }
    }
    (r == APP::ERROR::AppNoError) ? SignStatusReply() : Reject(r);
}

void TsiSp003AppVer21::EnDisPlan(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
    {
        return;
    }
    auto r = ctrller.CmdEnDisPlan(data);
    if (r == APP::ERROR::AppNoError)
    {
        if (data[0] == static_cast<uint8_t>(MI::CODE::EnablePlan))
        {
            db.GetUciEvent().Push(0, "EnablePlan: Group%d, Plan%d", data[1], data[2]);
        }
        else
        {
            db.GetUciEvent().Push(0, "DisablePlan: Group%d, Plan%d", data[1], data[2]);
        }
        Ack();
    }
    else
    {
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
    if (data[1] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    else if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 2 + data[1] * 3))
    {
        return;
    }
    auto r = ctrller.CmdSetDimmingLevel(data);
    if (r == APP::ERROR::AppNoError)
    {
        char buf[64];
        int len = sprintf(buf, "SignSetDimming");
        uint8_t *p = data + 2;
        for (int i = 0; i < data[1] && i < 4; i++)
        {
            len += snprintf(buf + len, 63 - len, ":Group%d(%d,%d)", p[0], p[1], p[2]);
            p += 3;
        }
        db.GetUciEvent().Push(0, buf);
        Ack();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::PowerOnOff(uint8_t *data, int len)
{
    if (data[1] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    else if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 2 + data[1] * 2))
    {
        return;
    }
    auto r = ctrller.CmdPowerOnOff(data, len);
    if (r == APP::ERROR::AppNoError)
    {
        char buf[64];
        int len = sprintf(buf, "Power");
        uint8_t *p = data + 2;
        for (int i = 0; i < data[1]; i++)
        {
            len += snprintf(buf + len, 63 - len, ":Group%d,%d", p[0], p[1]);
            p += 2;
        }
        db.GetUciEvent().Push(0, buf);
        Ack();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::DisableEnableDevice(uint8_t *data, int len)
{
    if (data[1] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    else if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 2 + data[1] * 2))
    {
        return;
    }
    auto r = ctrller.CmdDisableEnableDevice(data, len);
    if (r == APP::ERROR::AppNoError)
    {
        char buf[64];
        int len = sprintf(buf, "Dis/EnableDevice");
        uint8_t *p = data + 2;
        for (int i = 0; i < data[1]; i++)
        {
            len += snprintf(buf + len, 63 - len, ":Group%d,%d", p[0], p[1]);
            p += 2;
        }
        db.GetUciEvent().Push(0, buf);
        Ack();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 3))
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
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1))
    {
        return;
    }
    txbuf[0] = static_cast<uint8_t>(MI::CODE::SignExtendedStatusReply);
    txbuf[1] = IsOnline() ? 1 : 0;
    txbuf[2] = static_cast<uint8_t>(appErr);
    UciProd &prod = db.GetUciProd();
    memcpy(txbuf + 3, prod.MfcCode(), 10);
    Time2Buf(txbuf + 13);
    txbuf[20] = static_cast<uint8_t>(ctrller.ctrllerError.GetErrorCode());
    int scnt = prod.NumberOfSigns();
    txbuf[21] = scnt;
    uint8_t *p = &txbuf[22];
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
    db.GetUciFault().Reset();
    db.GetUciEvent().Push(0, "ResetFaultLog");
    Ack();
}
