#include <sign/Sign.h>
#include <uci/DbHelper.h>

Sign::Sign(uint8_t sid)
    : signId(sid), signErr(0), en_dis(1),
      reportFrmId(0), reportMsgId(0), reportPlnId(0)
{
}

uint8_t *Sign::GetStatus(uint8_t *p)
{
    DbHelper &db = DbHelper::Instance();
    *p++ = signId;
    *p++ = signErr;
    *p++ = en_dis;
    *p++ = reportFrmId;
    *p++ = db.uciFrm.GetFrm(reportFrmId)->frmRev;
    *p++ = reportMsgId;
    *p++ = db.uciMsg.GetMsg(reportMsgId)->msgRev;
    *p++ = reportPlnId;
    *p++ = db.uciPln.GetPln(reportPlnId)->plnRev;
    return p;
}

void Sign::SetDimming(uint8_t dimming_)
{
    if(dimming_<16)
    {
        dimming = dimming_;
    }
}
