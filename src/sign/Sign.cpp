#include <sign/Sign.h>
#include <uci/DbHelper.h>

const char *SIGNTYPE[SIGNTYPE_SIZE]={
    "VMS",     // 1 sign at 1 com, has 1-x slaves, slave id 1-x  
    "ISLUS",   //  1 group of ISLUS at 1 com, 1 sign has 1 slave, slave id 1
};

Sign::Sign(uint8_t sid)
    :signId(sid),errCode(0), en_dis(1),
    reportFrmId(0), reportMsgId(0), reportPlnId(0),
    dimMode(0), dimLevel(0)
{
}

uint8_t Sign::SignId()
{
    return signId;
}

uint8_t * Sign::GetStatus(uint8_t *p)
{
    DbHelper &db = DbHelper::Instance();
    *p++=signId;
    *p++=errCode;
    *p++=en_dis;
    *p++=reportFrmId;
    *p++=db.uciFrm.GetFrm(reportFrmId)->frmRev;
    *p++=reportMsgId;
    *p++=db.uciMsg.GetMsg(reportMsgId)->msgRev;;
    *p++=reportPlnId;
    *p++=db.uciPln.GetPln(reportPlnId)->plnRev;;
    return p;
}
