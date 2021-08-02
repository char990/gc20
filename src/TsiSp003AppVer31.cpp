#include "TsiSp003AppVer31.h"
#include "TsiSp003Const.h"
#include "DbHelper.h"
#include "Controller.h"


TsiSp003AppVer31::TsiSp003AppVer31()
{

}
TsiSp003AppVer31::~TsiSp003AppVer31()
{
    
}

int TsiSp003AppVer31::Rx(uint8_t * data, int len)
{
    micode = *data;
    switch (micode)
    {
    case MI_CODE::SignExtendedStatusRequest:
        SignExtendedStatusRequest(data, len);
        break;
    case MI_CODE::SignSetGraphicsFrame:
        SignSetGraphicsFrame(data, len);
        break;
    case MI_CODE::SignRequestStoredFMP:
        SignRequestStoredFMP(data, len);
        break;
    default:
        return TsiSp003AppVer10::Rx(data,len);
    }
    return 0;
}

void TsiSp003AppVer31::SignExtendedStatusRequest(uint8_t *data, int len)
{
    if (ChkLen(len, 1) == false)
        return;
    txbuf[0] = MI_CODE::SignExtendedStatusReply;
    txbuf[1] = online?1:0;
    txbuf[2] = appError;
    memcpy(txbuf+3, db.MfcCode(), 10);
    Time2Buf(txbuf+13);
    txbuf[20] = ctrl.ErrorCode();
    int scnt = ctrl.SignCnt();
    txbuf[21] = scnt;
    uint8_t *p = txbuf[22];
    for(int i=0;i<scnt;i++)
    {
        p=ctrl.signs[i].GetExtStatus(p);
    }
    Tx(buf, p-buf);
}

void TsiSp003AppVer31::SignSetGraphicsFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer31::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

