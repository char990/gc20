#include <cstring>
#include <sign/Sign.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
extern time_t GetTime(time_t *);

Sign::Sign(uint8_t id)
    : signId(id)
{
    UciProd &prod = DbHelper::Instance().GetUciProd();
    chainFault.SetCNT(prod.DriverFaultDebounce());
    multiLedFault.SetCNT(prod.LedFaultDebounce());
    singleLedFault.SetCNT(prod.LedFaultDebounce());
    selftestFault.SetCNT(prod.SelftestDebounce());
    voltageFault.SetCNT(prod.SlaveVoltageDebounce());
    lanternFault.SetCNT(prod.LanternFaultDebounce());
    overtempFault.SetCNT(prod.OverTempDebounce());

    lsConnectionFault.SetCNT(2 * 60, 3 * 60); // fault debounce 1 minute in slave, so set true_cnt as 2*60
    ls18hoursFault.SetCNT(18 * 60 * 60, 15 * 60);
    lsMidnightFault.SetCNT(15 * 60);
    lsMiddayFault.SetCNT(15 * 60);
}

Sign::~Sign()
{
}

void Sign::AddSlave(Slave *slave)
{
    vsSlaves.push_back(slave);
    slave->sign = this;
}

void Sign::InitFaults()
{
    luminanceFault.Init(signErr.IsSet(DEV::ERROR::SignLuminanceControllerFailure) ? STATE5::S5_1 : STATE5::S5_0);
    selftestFault.SetState(signErr.IsSet(DEV::ERROR::UnderLocalControl));
    singleLedFault.SetState(signErr.IsSet(DEV::ERROR::SignSingleLedFailure));
    lanternFault.SetState(signErr.IsSet(DEV::ERROR::ConspicuityDeviceFailure));
    voltageFault.SetState(signErr.IsSet(DEV::ERROR::InternalPowerSupplyFault));
    multiLedFault.SetState(signErr.IsSet(DEV::ERROR::SignMultiLedFailure));
    chainFault.SetState(signErr.IsSet(DEV::ERROR::SignDisplayDriverFailure));
    overtempFault.SetState(signErr.IsSet(DEV::ERROR::OverTemperatureAlarm));
    lsConnectionFault.Reset();
    ls18hoursFault.SetState(false);
    lsMidnightFault.SetState(false);
    lsMiddayFault.SetState(false);
    fatalError.Init((chainFault.IsHigh() ||
                     multiLedFault.IsHigh() ||
                     selftestFault.IsHigh() ||
                     voltageFault.IsHigh() ||
                     overtempFault.IsHigh())
                        ? STATE5::S5_1
                        : STATE5::S5_0);
}

void Sign::ClearFaults()
{
    signErr.Clear();
    chainFault.SetState(false);
    multiLedFault.SetState(false);
    singleLedFault.SetState(false);
    selftestFault.SetState(false);
    voltageFault.SetState(false);
    overtempFault.SetState(false);
    lanternFault.SetState(false);
    lsConnectionFault.SetState(false);
    ls18hoursFault.SetState(false);
    lsMidnightFault.SetState(false);
    lsMiddayFault.SetState(false);
    tflag = 255;
    luminanceFault.Init(STATE5::S5_0);
    fatalError.Init(STATE5::S5_0);
    signErr.Clear();
}

uint8_t *Sign::GetStatus(uint8_t *p)
{
    DbHelper &db = DbHelper::Instance();
    *p++ = signId;
    *p++ = static_cast<uint8_t>(signErr.GetErrorCode());
    *p++ = device;
    *p++ = reportFrm;
    *p++ = db.GetUciFrm().GetFrmRev(reportFrm);
    *p++ = reportMsg;
    *p++ = db.GetUciMsg().GetMsgRev(reportMsg);
    *p++ = reportPln;
    *p++ = db.GetUciPln().GetPlnRev(reportPln);
    return p;
}

void Sign::RefreshSlaveStatusAtSt()
{
    for (auto &s : vsSlaves)
    {
        if (s->rxStatus == 0)
        {
            return;
        }
    }

    // ----------------------Check status
    // single & multiLed bits ignored. checked in ext_st
    // over-temperature bits ignored. checked in ext_st
    char buf[64];
    uint8_t check_chain_fault = 0;
    uint8_t check_selftest = 0;
    uint8_t check_lantern = 0;

    for (auto &s : vsSlaves)
    {
        check_chain_fault |= s->panelFault & 0x0F;
        check_selftest |= s->selfTest & 1;
    }
    // lanterns installed at first&last slaves
    check_lantern = (vsSlaves.size() == 1) ? (vsSlaves[0]->lanternFan & 0x0F) : ((vsSlaves[0]->lanternFan & 0x03) | ((vsSlaves[vsSlaves.size() - 1]->lanternFan & 0x03) << 2));

    chainFault.Check(check_chain_fault > 0);
    DbncFault(chainFault, DEV::ERROR::SignDisplayDriverFailure);

    selftestFault.Check(check_selftest > 0);
    DbncFault(selftestFault, DEV::ERROR::UnderLocalControl);

    lanternFault.Check(check_lantern > 0);
    sprintf(buf, "LanternFault=0x%02X", check_lantern);
    DbncFault(lanternFault, DEV::ERROR::ConspicuityDeviceFailure, buf);

    RefreshFatalError();
}

void Sign::RefreshSlaveStatusAtExtSt()
{
    for (auto &s : vsSlaves)
    {
        if (s->rxExtSt == 0)
        {
            return;
        }
    }

    // ----------------------Check status
    // single & multiLed bits ignored. checked in ext_st
    // over-temperature bits ignored. checked in ext_st
    char buf[64];
    DbHelper &db = DbHelper::Instance();
    UciProd &prod = db.GetUciProd();
    UciUser &user = db.GetUciUser();

    // ----------------------Check ext-status
    uint16_t minvoltage = 0xFFFF, maxvoltage = 0; // mV
    uint32_t v = 0;
    uint8_t vcnt = 0;
    int16_t temperature = 0; // 0.1'C
    uint16_t faultLeds = 0;
    for (auto &s : vsSlaves)
    {
        if (s->voltage > 3000 && s->voltage < 15000)
        {
            v += s->voltage;
            vcnt++;
            if (s->voltage > maxvoltage)
            {
                maxvoltage = s->voltage;
            }
            if (s->voltage < minvoltage)
            {
                minvoltage = s->voltage;
            }
        }
        if (s->temperature > temperature)
        {
            temperature = s->temperature;
        }
        uint8_t *p = s->numberOfFaultyLed;
        for (int i = 0; i < Slave::numberOfTiles * Slave::numberOfColours; i++)
        {
            faultLeds += *p++;
        }
    }

    // *** voltage
    if (minvoltage < prod.SlaveVoltageLow())
    {
        voltageFault.Check(true);
        voltage = minvoltage;
    }
    else if (maxvoltage > prod.SlaveVoltageHigh())
    {
        voltageFault.Check(true);
        voltage = maxvoltage;
    }
    else
    {
        voltageFault.Check(false);
        voltage = v / vcnt;
    }
    sprintf(buf, "%dmv", voltage);
    DbncFault(voltageFault, DEV::ERROR::InternalPowerSupplyFault, buf);

    // *** temperature
    curTemp = temperature / 10;
    if (curTemp > maxTemp)
    {
        maxTemp = curTemp;
    }
    auto ot = user.OverTemp();
    if (curTemp > ot)
    {
        overtempFault.Check(1);
        if (!signErr.IsSet(DEV::ERROR::OverTemperatureAlarm) && overtempFault.IsHigh())
        {
            signErr.Push(signId, DEV::ERROR::OverTemperatureAlarm, true);
            DbHelper::Instance().GetUciAlarm().Push(signId, "Sign%d OverTemperatureAlarm ONSET: %d'C", signId, curTemp);
            overtempFault.ClearEdge();
        }
    }
    else if (curTemp < (ot - 3))
    {
        overtempFault.Check(0);
        if (signErr.IsSet(DEV::ERROR::OverTemperatureAlarm) && overtempFault.IsLow())
        {
            signErr.Push(signId, DEV::ERROR::OverTemperatureAlarm, false);
            DbHelper::Instance().GetUciAlarm().Push(signId, "Sign%d OverTemperatureAlarm CLEAR: %d'C", signId, curTemp);
            overtempFault.ClearEdge();
        }
    }

    // *** single/multi led
    if (faultLeds == 0)
    {
        singleLedFault.Check(0);
        multiLedFault.Check(0);
    }
    else if (faultLeds == 1)
    {
        singleLedFault.Check(1);
        multiLedFault.Check(0);
    }
    else
    {
        singleLedFault.Check(1);
        multiLedFault.Check(faultLeds > user.MultiLedFaultThreshold());
    }
    if (multiLedFault.IsRising() || multiLedFault.IsFalling())
    {
        faultLedCnt = faultLeds;
    }
    sprintf(buf, "%d LEDs", faultLeds);
    DbncFault(multiLedFault, DEV::ERROR::SignMultiLedFailure, buf);
    if (multiLedFault.IsLow())
    {
        DbncFault(singleLedFault, DEV::ERROR::SignSingleLedFailure, buf);
        if (singleLedFault.IsRising() || singleLedFault.IsFalling())
        {
            faultLedCnt = faultLeds;
        }
    }
    else
    {
        singleLedFault.ClearEdge();
    }
    RefreshFatalError();

    // *** light sensor
    auto t = GetTime(nullptr);
    // light sensor installed at first slave
    auto lscon = lsConnectionFault.Value();
    lux = vsSlaves[0]->lux;
    if ((vsSlaves[0]->lightSensorFault & 1) == 0 && lux > 0)
    {
        lsConnectionFault.Clr();
    }
    else
    {
        lsConnectionFault.Check(vsSlaves[0]->lightSensorFault & 1, t);
    }
    if (lsConnectionFault.IsRising())
    {
        lsConnectionFault.ClearRising();
        luminanceFault.Set();
        signErr.Push(signId, DEV::ERROR::SignLuminanceControllerFailure, true);
        db.GetUciAlarm().Push(signId, "Light sensor DISCONNECTED");
    }
    else if (lsConnectionFault.IsFalling())
    {
        lsConnectionFault.ClearFalling();
        if (lscon != STATE5::S5_NA)
        {
            db.GetUciAlarm().Push(signId, "Light sensor CONNECTED");
        }
    }

    if (lsConnectionFault.IsLow())
    {
        auto &prod = DbHelper::Instance().GetUciProd();
        ls18hoursFault.Check(lux < prod.LightSensor18Hours(), t);
        struct tm stm;
        localtime_r(&t, &stm);
        if (stm.tm_hour >= 11 && stm.tm_hour < 15)
        { // mid-day
            if (lasthour != stm.tm_hour && (lasthour < 11 || lasthour > 15))
            {
                lsMiddayFault.ResetCnt();
            }
            lsMiddayFault.Check(lux < prod.LightSensorMidday(), t);
        }
        if (stm.tm_hour < 3 || stm.tm_hour >= 23)
        { // mid-night
            if (lasthour != stm.tm_hour && (lasthour < 23 || lasthour > 3))
            {
                lsMidnightFault.ResetCnt();
            }
            lsMidnightFault.Check(lux > prod.LightSensorMidnight(), t);
        }
        lasthour = stm.tm_hour;
        if (ls18hoursFault.IsHigh() ||
            lsMiddayFault.IsHigh() ||
            lsMidnightFault.IsHigh())
        { // any failed
            if (!luminanceFault.IsHigh())
            {
                signErr.Push(signId, DEV::ERROR::SignLuminanceControllerFailure, true);
                luminanceFault.Set();
            }
        }
        else if (ls18hoursFault.IsLow() &&
                 lsMiddayFault.IsLow() &&
                 lsMidnightFault.IsLow())
        { // all good
            if (!luminanceFault.IsLow())
            {
                signErr.Push(signId, DEV::ERROR::SignLuminanceControllerFailure, false);
                luminanceFault.Clr();
            }
        }
        if (ls18hoursFault.HasEdge())
        {
            ls18hoursFault.ClearEdge();
            if (ls18hoursFault.IsHigh())
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)<%d for 18-hour: ls18hours ONSET", lux, prod.LightSensor18Hours());
            }
            else
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)>=%d for 15-min: ls18hours CLEAR", lux, prod.LightSensor18Hours());
            }
        }
        if (lsMiddayFault.HasEdge())
        {
            lsMiddayFault.ClearEdge();
            if (lsMiddayFault.IsHigh())
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)<%d for 15-min in 11am-3pm: lsMidday ONSET", lux, prod.LightSensorMidday());
            }
            else
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)>=%d for 15-minin 11am-3pm: lsMidday CLEAR", lux, prod.LightSensorMidday());
            }
        }
        if (lsMidnightFault.HasEdge())
        {
            lsMidnightFault.ClearEdge();
            if (lsMidnightFault.IsHigh())
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)>=%d for 15-min in 23pm-3am: lsMidnight ONSET", lux, prod.LightSensorMidnight());
            }
            else
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)<%d for 15-min in 23pm-3am: lsMidnight CLEAR", lux, prod.LightSensorMidnight());
            }
        }
    }
}

void Sign::RefreshFatalError()
{
    if (chainFault.IsHigh() || multiLedFault.IsHigh() || selftestFault.IsHigh() || voltageFault.IsHigh() || overtempFault.IsHigh())
    {
        fatalError.Set();
        if (fatalError.IsRising())
        {
            PrintDbg(DBG_LOG, "Sign[%d] fatalError Set", signId);
        }
    }
    else
    {
        fatalError.Clr();
        if (fatalError.IsFalling() && (fatalError.PreV() != STATE5::S5_NA))
        {
            PrintDbg(DBG_LOG, "Sign[%d] fatalError Clr", signId);
        }
    }
    fatalError.ClearEdge();
}

uint8_t *Sign::LedStatus(uint8_t *buf)
{
    uint8_t tiles = SignTiles();
    uint8_t tbytes = (tiles + 7) / 8;
    memset(buf, 0, tbytes);
    int bitOffset = 0;
    for (auto &s : vsSlaves)
    {
        for (int i = 0; i < s->numberOfTiles; i++, bitOffset++)
        {
            for (int j = 0; j < s->numberOfColours; j++)
            {
                if (*(s->numberOfFaultyLed + j * s->numberOfTiles + i) > 0)
                {
                    BitOffset::SetBit(buf, bitOffset);
                    break;
                }
            }
        }
    }
    return buf + tbytes;
}

void Sign::DbncFault(Debounce &dbc, DEV::ERROR err, const char *info)
{
    char buf[64];
    int len = 0;
    if (dbc.IsRising())
    {
        if (!signErr.IsSet(err))
        {
            signErr.Push(signId, err, true);
            len = snprintf(buf, 63, "Sign%d %s ONSET", signId, DEV::ToStr(err));
        }
    }
    else if (dbc.IsFalling())
    {
        if (signErr.IsSet(err))
        {
            signErr.Push(signId, err, false);
            len = snprintf(buf, 63, "Sign%d %s CLEAR", signId, DEV::ToStr(err));
        }
    }
    dbc.ClearEdge();
    if (len > 0)
    {
        if (info != nullptr)
        {
            snprintf(buf + len, 63 - len, ": %s", info);
        }
        DbHelper::Instance().GetUciAlarm().Push(signId, buf);
    }
}
