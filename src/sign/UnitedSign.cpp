#include <sign/UnitedSign.h>
#include <uci/DbHelper.h>

void UnitedSign::DispNext(DISP_STATUS::TYPE type,uint8_t id)
{
    if(type==DISP_STATUS::TYPE::EXT)
    {
        dsExt.dispType = type;
        dsExt.fmpid = id;
    }
    else
    {
        dsNext.dispType = type;
        dsNext.fmpid = id;
    }
}

uint8_t *Sign::GetStatus(uint8_t *p)
{
    DbHelper &db = DbHelper::Instance();
    *p++ = signId;
    *p++ = signErr;
    *p++ = en_dis;
    *p++ = currentFrm;
    *p++ = db.uciFrm.GetFrmRev(reportFrmId);
    *p++ = currentMsg;
    *p++ = db.uciMsg.GetMsgRev(reportMsgId);
    *p++ = currentPln;
    *p++ = db.uciPln.GetPlnRev(reportPlnId);
    return p;
}
