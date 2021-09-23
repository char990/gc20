#include <sign/Sign.h>
#include <uci/DbHelper.h>

Sign::Sign(uint8_t id) 
:signId(id),signErr(0),
dimmingSet(0),dimmingV(1),
deviceSet(1),deviceV(1),
currentFrm(0),currentMsg(0),currentPln(0),
lightsensor(DbHelper::Instance().uciProd.LightSensorFaultDebounce(),DbHelper::Instance().uciProd.LightSensorFaultDebounce())
{
    UciProd &prod = DbHelper::Instance().uciProd;
    numberOfSlaves = prod.SlaveRowsPerSign() * prod.SlaveColumnsPerSign();
    slaves = new Slave[numberOfSlaves];
    for(int i=0;i<numberOfSlaves;i++)
    {
        slaves[i].slaveId=i+1;
    }
}

Sign::~Sign()
{
    delete[] slaves;
}

void Sign::Reset()
{
    CurrentDisp(0, 0, 0);
    lightsensor.Reset();
}

uint8_t *Sign::GetStatus(uint8_t *p)
{
    DbHelper &db = DbHelper::Instance();
    *p++ = signId;
    *p++ = signErr;
    *p++ = deviceV;
    *p++ = currentFrm;
    *p++ = db.uciFrm.GetFrmRev(currentFrm);
    *p++ = currentMsg;
    *p++ = db.uciMsg.GetMsgRev(currentFrm);
    *p++ = currentPln;
    *p++ = db.uciPln.GetPlnRev(currentFrm);
    return p;
}
