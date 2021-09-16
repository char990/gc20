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
    case MI::CODE::PowerONOFF:
        PowerONOFF(data, len);
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
        ;
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
    txbuf[12] = Scheduler::Instance().CtrllerErr();
    int scnt = DbHelper::Instance().uciProd.NumberOfSigns();
    txbuf[13] = scnt;
    uint8_t *p = &txbuf[14];
    for (int i = 1; i <= scnt; i++)
    {
        p = Scheduler::Instance().GetUnitedSign(i)->GetStatus(p);
    }
    Tx(txbuf, p - txbuf);
}

void TsiSp003AppVer21::SignSetFrame(uint8_t *data, int len)
{
    APP::ERROR r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_FRM_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (Scheduler::Instance().IsFrmActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else if (id <= DbHelper::Instance().uciUser.LockedFrm()) // && pstatus->rFSstate() != Status::FS_OFF)
    {
        r = APP::ERROR::OverlaysNotSupported; // pre-defined and can't be overlapped
    }
    else
    {
        UciFrm &uci = DbHelper::Instance().uciFrm;
        r = uci.SetFrm(data, len);
        if (r == APP::ERROR::AppNoError)
        {
            uci.SaveFrm(id);
        }
    }
    if (r == APP::ERROR::AppNoError)
    {
        SignStatusReply();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::SignSetTextFrame(uint8_t *data, int len)
{
    SignSetFrame(data, len);
}

void TsiSp003AppVer21::SignSetGraphicsFrame(uint8_t *data, int len)
{
    SignSetFrame(data, len);
}

void TsiSp003AppVer21::SignDisplayFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::SignSetMessage(uint8_t *data, int len)
{
    APP::ERROR r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_MSG_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (len > MSG_LEN_MAX)
    {
        r = APP::ERROR::LengthError;
    }
    else if (Scheduler::Instance().IsMsgActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else if (id <= DbHelper::Instance().uciUser.LockedMsg()) // && pstatus->rFSstate() != Status::FS_OFF)
    {
        r = APP::ERROR::OverlaysNotSupported; // pre-defined and can't be overlapped
    }
    else
    {
        Cnvt::PutU16(Crc::Crc16_1021(data, len), data + len); // attach CRC
        UciMsg &uci = DbHelper::Instance().uciMsg;
        r = uci.SetMsg(data, len + 2);
        if (r == APP::ERROR::AppNoError)
        {
            uci.SaveMsg(id);
        }
    }
    if (r == APP::ERROR::AppNoError)
    {
        SignStatusReply();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::SignDisplayMessage(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::SignSetPlan(uint8_t *data, int len)
{
    APP::ERROR r = APP::ERROR::AppNoError;
    uint8_t id = *(data + OFFSET_PLN_ID);
    if (id == 0)
    {
        r = APP::ERROR::FrmMsgPlnUndefined;
    }
    else if (len > PLN_LEN_MAX)
    {
        r = APP::ERROR::LengthError;
    }
    else if (Scheduler::Instance().IsPlnActive(id))
    {
        r = APP::ERROR::FrmMsgPlnActive;
    }
    else
    {
        Cnvt::PutU16(Crc::Crc16_1021(data, len), data + len); // attach CRC
        data[len + 2] = 0;                                    // new plan is disabled
        UciPln &uci = DbHelper::Instance().uciPln;
        r = uci.SetPln(data, len + 3);
        if (r == APP::ERROR::AppNoError)
        {
            uci.SavePln(id);
        }
    }
    if (r == APP::ERROR::AppNoError)
    {
        SignStatusReply();
    }
    else
    {
        Reject(r);
    }
}

void TsiSp003AppVer21::EnablePlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::DisablePlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::RequestEnabledPlans(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::PowerONOFF(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::DisableEnableDevice(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer21::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    if (data[1] > 3 || data[2] == 0)
    {
        Reject(APP::ERROR::SyntaxError);
    }
    switch (data[1])
    {
    case 0: // frm
    {
        StFrm *frm = DbHelper::Instance().uciFrm.GetFrm(data[2]);
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
        Message *msg = DbHelper::Instance().uciMsg.GetMsg(data[2]);
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
        Plan *pln = DbHelper::Instance().uciPln.GetPln(data[2]);
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
    if (ChkLen(len, 1) == false)
        return;
    txbuf[0] = MI::CODE::SignExtendedStatusReply;
    txbuf[1] = IsOnline() ? 1 : 0;
    txbuf[2] = appErr;
    memcpy(txbuf + 3, db.uciProd.MfcCode(), 10);
    Time2Buf(txbuf + 13);
    txbuf[20] = Scheduler::Instance().CtrllerErr();
    int scnt = db.uciProd.NumberOfSigns();
    txbuf[21] = scnt;
    uint8_t *p = &txbuf[22];
    for (int i = 1; i <= scnt; i++)
    {
        p = Scheduler::Instance().GetUnitedSign(i)->GetExtStatus(p);
    }
    int applen = p - txbuf;
    uint16_t crc = Crc::Crc16_1021(txbuf, applen);
    txbuf[applen++] = crc >> 8;
    txbuf[applen++] = crc;
    Tx(txbuf, applen);
}
