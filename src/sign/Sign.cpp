#include <sign/Sign.h>
#include <uci/DbHelper.h>

const char *SIGNTYPE[SIGNTYPE_SIZE]={
    "TypeC1",     // typec, 288*64, 6 slaves, 4-bit 
    "MiniVms1",    // minivms(floodsign), 64*25, 1 slave, amber
};


SignCfg Sign::signCfg;

Sign::Sign() 
    :groupId(0), signId(0), errCode(0), en_dis(1),
    reportFrmId(0), reportMsgId(0), reportPlnId(0),
    dimMode(0), dimLevel(0)
{
    
}

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
    *p++=db.uciMsg.GetMsg(reportMsgId)->msgRev;;
    *p++=reportPlnId;
    *p++=db.uciPln.GetPln(reportPlnId)->plnRev;;
    return p;
}
