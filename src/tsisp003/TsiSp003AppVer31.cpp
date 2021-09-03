#include <cstring>

#include <tsisp003/TsiSp003AppVer31.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <module/Controller.h>
#include <module/Utils.h>

using namespace Utils;

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
    case MI::CODE::SignExtendedStatusRequest:
        SignExtendedStatusRequest(data, len);
        break;
    case MI::CODE::SignSetGraphicsFrame:
        SignSetGraphicsFrame(data, len);
        break;
    case MI::CODE::SignRequestStoredFMP:
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
    txbuf[0] = MI::CODE::SignExtendedStatusReply;
    txbuf[1] = IsOnline() ? 1 : 0 ;
    txbuf[2] = appError;
    memcpy(txbuf+3, db.uciProd.MfcCode(), 10);
    Time2Buf(txbuf+13);
    txbuf[20] = scheduler.ErrorCode();
    int scnt = DbHelper::Instance().uciProd.NumberOfSigns();
    txbuf[21] = scnt;
    uint8_t *p = &txbuf[22];
    for(int i=0;i<scnt;i++)
    {
        p=scheduler.signs[i]->GetExtStatus(p);
    }
    int applen = p-txbuf;
    uint16_t crc = Crc::Crc16_1021(txbuf, applen);
    txbuf[applen++]=crc>>8;
    txbuf[applen++]=crc;
    Tx(txbuf, applen);
}

void TsiSp003AppVer31::SignSetGraphicsFrame(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

void TsiSp003AppVer31::SignRequestStoredFMP(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP::ERROR::SyntaxError);
    Ack();
}

