#include <ctime>
#include "TsiSp003AppVer10.h"
#include "TsiSp003Const.h"
#include "DbHelper.h"
#include "Controller.h"

using namespace Util;
using namespace std;

TsiSp003AppVer10::TsiSp003AppVer10()
{

}
TsiSp003AppVer10::~TsiSp003AppVer10()
{
    
}

void TsiSp003AppVer10::Time2Buf(uint8_t *p);
{
    struct tm t;
    localtime_r(time(nullptr), &t);
    *p++=t.tm_mday;
    *p++=t.tm_mon+1;
    int year = t.tm_year+1900;
    *p++=year>>8;
    *p++=year&0xFF;
    *p++=t.tm_hour;
    *p++=t.tm_min;
    *p++=t.tm_sec;
}

int TsiSp003AppVer10::Rx(uint8_t * data, int len)
{
    micode = *data;
    switch (micode)
    {
    case MI_CODE::HeartbeatPoll:
        HeartbeatPoll(data, len);
        break;
    case MI_CODE::SignSetTextFrame:
        SignSetTextFrame(data, len);
        break;
    case MI_CODE::SignDisplayFrame:
        SignDisplayFrame(data, len);
        break;
    case MI_CODE::SignSetMessage:
        SignSetMessage(data, len);
        break;
    case MI_CODE::SignDisplayMessage:
        SignDisplayMessage(data, len);
        break;
    case MI_CODE::SignSetPlan:
        SignSetPlan(data, len);
        break;
    case MI_CODE::EnablePlan:
        EnablePlan(data, len);
        break;
    case MI_CODE::DisablePlan:
        DisablePlan(data, len);
        break;
    case MI_CODE::RequestEnabledPlans:
        RequestEnabledPlans(data, len);
        break;
    case MI_CODE::SignRequestStoredFMP:
        SignRequestStoredFMP(data, len);
        break;
    default:
        return -1;
    }
    return 0;
}

void TsiSp003AppVer10::HeartbeatPoll(uint8_t *data, int len)
{
    if (ChkLen(len, 1) == false)
        return;
    txbuf[0] = MI_CODE::SignStatusReply;
    txbuf[1] = online?1:0;
    txbuf[2] = appError;
    Time2Buf(txbuf+3);
    uint16_t i16 = db.HdrChksum();
    txbuf[10] = i16>>8;
    txbuf[11] = i16&0xff;
    txbuf[12] = ctrl.ErrorCode();
    int scnt = ctrl.SignCnt();
    txbuf[13] = scnt;
    uint8_t *p = txbuf[14];
    for(int i=0;i<scnt;i++)
    {
        p=ctrl.signs[i].GetStatus(p);
    }
    Tx(buf, p-buf);
}

void TsiSp003AppVer10::SignSetTextFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignDisplayFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignSetMessage(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignDisplayMessage(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignSetPlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::EnablePlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::DisablePlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::RequestEnabledPlans(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

