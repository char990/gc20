#include "TsiSp003AppVer10.h"
#include "TsiSp003Const.h"
#include "DbHelper.h"
#include "Controller.h"

TsiSp003AppVer10::TsiSp003AppVer10()
{

}
TsiSp003AppVer10::~TsiSp003AppVer10()
{
    
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
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
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

