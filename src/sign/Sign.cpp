#include <cstring>
#include <sign/Sign.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>

Sign::Sign(uint8_t id)
    : signId(id), signErr(0),
      dimmingSet(0), dimmingV(1),
      deviceSet(1), deviceV(1),
      reportFrm(0), reportMsg(0), reportPln(0)
{
    UciProd & prod = DbHelper::Instance().GetUciProd();
    dbcLight.SetCNT(prod.LightSensorFaultDebounce());
    dbcChain.SetCNT(prod.DriverFaultDebounce());
    dbcMultiLed.SetCNT(prod.LedFaultDebounce());
    dbcSingleLed.SetCNT(prod.LedFaultDebounce());
    dbcOverTemp.SetCNT(prod.OverTempDebounce());
    dbcSelftest.SetCNT(prod.SelftestDebounce());
    dbcFan.SetCNT(prod.LanternFaultDebounce());
    dbcVoltage.SetCNT(prod.SlaveVoltageDebounce());
    dbcLantern.SetCNT(prod.LanternFaultDebounce());
}

Sign::~Sign()
{
}

void Sign::AddSlave(Slave * slave) 
{
    vsSlaves.push_back(slave);
}

void Sign::Reset()
{
    CurrentDisp(0, 0, 0);
    dbcLight.Reset();
    dbcChain.Reset();
    dbcMultiLed.Reset();
    dbcSingleLed.Reset();
    dbcOverTemp.Reset();
    dbcSelftest.Reset();
    dbcFan.Reset();
    dbcVoltage.Reset();
    dbcLantern.Reset();
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

void Sign::RefreshSlaveStatus() 
{
	// single & multiLed bits ignored. checked in ext_st
	// over-temperature bits ignored. checked in ext_st
    uint8_t check_chain_fault=0;
	uint8_t check_selftest=0;
	uint8_t check_lightsensor=0;
	uint8_t check_fan=0;

    for (auto s : vsSlaves)
    {
        check_chain_fault |= s->panelFault & 0x0F;
        check_selftest |= s->selfTest & 1;
        check_lightsensor |= s->lightSensorFault & 1;
        check_fan |= s->lanternFan & 0x30;
    }
	uint8_t check_lantern = (vsSlaves.size()==1)?
            (vsSlaves[0]->lanternFan & 0x0F) :
            ((vsSlaves[0]->lanternFan & 0x03) | ((vsSlaves[vsSlaves.size()-1]->lanternFan & 0x03)<<2));

    dbcChain.Check(check_chain_fault>0);
    dbcSelftest.Check(check_selftest>0);
    dbcFan.Check(check_fan>0);
    dbcLight.Check(check_lightsensor>0);
    dbcLantern.Check(check_lantern>0);
}

void Sign::RefreshSlaveExtSt() 
{
    uint16_t minvoltage=0xFFFF, maxvoltage=0;       // mV
    uint16_t temperature=0;   // 0.1'C
    uint16_t faultLedCnt=0;

    for (auto s : vsSlaves)
    {
        if(s->voltage > maxvoltage)
        {
            maxvoltage = s->voltage;
        }
        if(s->voltage < minvoltage)
        {
            minvoltage = s->voltage;
        }
        if(s->temperature > temperature)
        {
            temperature = s->temperature;
        }
        uint8_t* p = s->numberOfFaultyLed;
        for(int i=0;i<Slave::numberOfTiles*Slave::numberOfColours;i++)
        {
            faultLedCnt+=*p++;
        }
    }

    DbHelper & db = DbHelper::Instance();
    UciProd &prod =  db.GetUciProd();
    dbcVoltage.Check(minvoltage<prod.SlaveVoltageLow() || maxvoltage>prod.SlaveVoltageHigh());
    UciUser & user = db.GetUciUser();
    auto ot = user.OverTemp();
    temperature/=10;
    if(temperature>ot)
    {
        dbcOverTemp.Check(1);
    }
    else if(temperature<(ot-3))
    {
        dbcOverTemp.Check(0);
    }

    if(faultLedCnt==0)
    {
        dbcSingleLed.Check(0);
        dbcMultiLed.Check(0);
    }
    else if (faultLedCnt==1)
    {
        dbcSingleLed.Check(1);
        dbcMultiLed.Check(0);
    }
    else
    {
        dbcSingleLed.Check(1);
        dbcMultiLed.Check(faultLedCnt>user.MultiLedFaultThreshold());
    }
}

uint8_t * Sign::LedStatus(uint8_t * buf) 
{
    uint8_t tiles = SignTiles();
    uint8_t tbytes = (tiles+7)/8;
    memset(buf, 0, tbytes);
    int bitOffset=0;
    for (auto s : vsSlaves)
    {
        for(int i=0;i<s->numberOfTiles;i++)
        {
            uint8_t tile=0;
            for(int j=0;j<s->numberOfColours;j++)
            {
                if(*(s->numberOfFaultyLed + j*s->numberOfTiles + i) > 0)
                {
                    tile=1;
                    break;
                }
            }
            if(tile!=0)
            {
                Utils::BitOffset::SetBit(buf, bitOffset);
            }
            bitOffset++;
        }        
    }
    return buf+tbytes;
}

