#include <sign/Sign.h>
#include <uci/DbHelper.h>

Sign::Sign(uint8_t id)
    : signId(id), signErr(0),
      dimmingSet(0), dimmingV(1),
      deviceSet(1), deviceV(1),
      reportFrm(0), reportMsg(0), reportPln(0),
      lightsensor(DbHelper::Instance().GetUciProd().LightSensorFaultDebounce(), DbHelper::Instance().GetUciProd().LightSensorFaultDebounce())
{
}

Sign::~Sign()
{
}

void Sign::Reset()
{
    CurrentDisp(0, 0, 0);
    lightsensor.Reset();
}

uint8_t *Sign::GetStatus(uint8_t *p)
{
    DbHelper & db = DbHelper::Instance();
    *p++ = signId;
    *p++ = signErr;
    *p++ = deviceV;
    *p++ = reportFrm;
    *p++ = db.GetUciFrm().GetFrmRev(reportFrm);
    *p++ = reportMsg;
    *p++ = db.GetUciMsg().GetMsgRev(reportFrm);
    *p++ = reportPln;
    *p++ = db.GetUciPln().GetPlnRev(reportFrm);
    return p;
}
