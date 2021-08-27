#include <sign/Sign.h>
#include <uci/DbHelper.h>

SignCfg Sign::signCfg;

void Sign::SetId(uint8_t sid, uint8_t gid)
{
    signId = sid;
    groupId = gid;
}

uint8_t Sign::SignId()
{
    return signId;
}


uint8_t Sign::GroupId()
{
    return groupId;
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
    *p++=db.uciFrm.GetMsg(reportMsgId)->msgRev;;
    *p++=reportPlnId;
    *p++=db.uciFrm.GetPln(reportPlnId)->plnRev;;
    return p;
}
