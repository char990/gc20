#include <ctime>
#include <tsisp003/TsiSp003AppVer10.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <module/Controller.h>
#include <module/Utils.h>

using namespace Utils;
using namespace std;

TsiSp003AppVer10::TsiSp003AppVer10()
{

}
TsiSp003AppVer10::~TsiSp003AppVer10()
{
    
}

void TsiSp003AppVer10::Time2Buf(uint8_t *p)
{
    struct tm t;
    time_t t_=time(nullptr);
    localtime_r(&t_, &t);
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
    case MI::CODE::HeartbeatPoll:
        HeartbeatPoll(data, len);
        break;
    case MI::CODE::SignSetTextFrame:
        SignSetTextFrame(data, len);
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
    case MI::CODE::SignRequestStoredFMP:
        SignRequestStoredFMP(data, len);
        break;
    default:
        return TsiSp003App::Rx(data,len);;
    }
    return 0;
}

void TsiSp003AppVer10::HeartbeatPoll(uint8_t *data, int len)
{
    if (ChkLen(len, 1) == false)
        return;
    txbuf[0] = MI::CODE::SignStatusReply;
    txbuf[1] = IsOnline()?1:0;
    txbuf[2] = appError;
    Time2Buf(txbuf+3);
    uint16_t i16 = db.HdrChksum();
    txbuf[10] = i16>>8;
    txbuf[11] = i16&0xff;
    txbuf[12] = ctrl.ErrorCode();
    int scnt = ctrl.SignCnt();
    txbuf[13] = scnt;
    uint8_t *p = &txbuf[14];
    for(int i=0;i<scnt;i++)
    {
        p=ctrl.signs[i]->GetStatus(p);
    }
    Tx(txbuf, p-txbuf);
}

void TsiSp003AppVer10::SignSetTextFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignDisplayFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignSetMessage(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignDisplayMessage(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignSetPlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::EnablePlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::DisablePlan(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::RequestEnabledPlans(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer10::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

