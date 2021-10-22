#include <cstring>
#include <sign/Sign.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

Sign::Sign(uint8_t id)
    : signId(id)
{
    UciProd &prod = DbHelper::Instance().GetUciProd();
    dbcChain.SetCNT(prod.DriverFaultDebounce());
    dbcMultiLed.SetCNT(prod.LedFaultDebounce());
    dbcSingleLed.SetCNT(prod.LedFaultDebounce());
    dbcSelftest.SetCNT(prod.SelftestDebounce());
    dbcVoltage.SetCNT(prod.SlaveVoltageDebounce());
    dbcLantern.SetCNT(prod.LanternFaultDebounce());

    dbncLightSnsr.SetCNT(2 * 60, 2 * 60);

    dbnc18hours.SetCNT(18 * 60 * 60, 15 * 60);
    dbnc18hours.SetState(false);
    dbnc18hours.changed = false;

    dbncMidnight.SetCNT(15 * 60);
    dbncMidnight.SetState(false);
    dbncMidnight.changed = false;

    dbncMidday.SetCNT(15 * 60);
    dbncMidday.SetState(false);
    dbncMidday.changed = false;
}

Sign::~Sign()
{
}

void Sign::AddSlave(Slave *slave)
{
    vsSlaves.push_back(slave);
}

void Sign::ClearFaults()
{
    signErr.Reset();
    dbcChain.Reset();
    dbcMultiLed.Reset();
    dbcSingleLed.Reset();
    dbcSelftest.Reset();
    dbcVoltage.Reset();
    dbcLantern.Reset();
    dbnc18hours.Reset();
    dbncMidnight.Reset();
    dbncMidday.Reset();
    tflag = 255;
    lightSnsrFault = STATE3::S_NA;
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

void Sign::RefreshSlaveStatusAtExtSt()
{
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
    // light sensor installed at first&last slaves
    check_lantern = (vsSlaves.size() == 1) ? (vsSlaves[0]->lanternFan & 0x0F) : ((vsSlaves[0]->lanternFan & 0x03) | ((vsSlaves[vsSlaves.size() - 1]->lanternFan & 0x03) << 2));

    dbcChain.Check(check_chain_fault > 0);
    DbncFault(dbcChain, DEV::ERROR::SignDisplayDriverFailure);

    dbcSelftest.Check(check_selftest > 0);
    DbncFault(dbcSelftest, DEV::ERROR::UnderLocalControl);

    dbcLantern.Check(check_lantern > 0);
    sprintf(buf, "LanternFault=0x%02X", check_lantern);
    DbncFault(dbcLantern, DEV::ERROR::ConspicuityDeviceFailure, buf);

    // ----------------------Check ext-status
    uint16_t minvoltage = 0xFFFF, maxvoltage = 0; // mV
    int16_t temperature = 0;                      // 0.1'C
    uint16_t faultLedCnt = 0;
    uint32_t v = 0;
    for (auto &s : vsSlaves)
    {
        v += s->voltage;
        if (s->voltage > maxvoltage)
        {
            maxvoltage = s->voltage;
        }
        if (s->voltage < minvoltage)
        {
            minvoltage = s->voltage;
        }
        if (s->temperature > temperature)
        {
            temperature = s->temperature;
        }
        uint8_t *p = s->numberOfFaultyLed;
        for (int i = 0; i < Slave::numberOfTiles * Slave::numberOfColours; i++)
        {
            faultLedCnt += *p++;
        }
    }

    // ------------------ debounce
    DbHelper &db = DbHelper::Instance();
    UciProd &prod = db.GetUciProd();
    UciUser &user = db.GetUciUser();

    // *** voltage
    if (minvoltage < prod.SlaveVoltageLow())
    {
        dbcVoltage.Check(true);
        voltage = minvoltage;
    }
    else if (maxvoltage > prod.SlaveVoltageHigh())
    {
        dbcVoltage.Check(true);
        voltage = maxvoltage;
    }
    else
    {
        dbcVoltage.Check(false);
        voltage = v / vsSlaves.size();
    }
    sprintf(buf, "%dmv", voltage);
    DbncFault(dbcVoltage, DEV::ERROR::InternalPowerSupplyFault, buf);

    // *** temperature
    curTemp = temperature / 10;
    if (curTemp > maxTemp)
    {
        maxTemp = curTemp;
    }
    auto ot = user.OverTemp();
    if (curTemp > ot)
    {
        if (!signErr.IsSet(DEV::ERROR::OverTemperatureAlarm))
        {
            signErr.Push(signId, DEV::ERROR::OverTemperatureAlarm, true);
            sprintf(buf, "Sign%d OverTemperatureAlarm ONSET: %d'C", signId, curTemp);
            DbHelper::Instance().GetUciAlarm().Push(signId, buf);
            overTempFault = Utils::STATE3::S_1;
        }
    }
    else if (curTemp < (ot - prod.OverTempDebounce()))
    {
        if (signErr.IsSet(DEV::ERROR::OverTemperatureAlarm))
        {
            signErr.Push(signId, DEV::ERROR::OverTemperatureAlarm, false);
            sprintf(buf, "Sign%d OverTemperatureAlarm CLEAR: %d'C", signId, curTemp);
            DbHelper::Instance().GetUciAlarm().Push(signId, buf);
            overTempFault = Utils::STATE3::S_0;
        }
    }

    // *** single/multi led
    if (faultLedCnt == 0)
    {
        dbcSingleLed.Check(0);
        dbcMultiLed.Check(0);
    }
    else if (faultLedCnt == 1)
    {
        dbcSingleLed.Check(1);
        dbcMultiLed.Check(0);
    }
    else
    {
        dbcSingleLed.Check(1);
        dbcMultiLed.Check(faultLedCnt > user.MultiLedFaultThreshold());
    }
    sprintf(buf, "%d LEDs", faultLedCnt);
    DbncFault(dbcMultiLed, DEV::ERROR::SignMultiLedFailure, buf);
    if (dbcMultiLed.Value() == STATE3::S_0)
    {
        DbncFault(dbcSingleLed, DEV::ERROR::SignSingleLedFailure, buf);
    }
    else
    {
        dbcSingleLed.changed = false; // When multi, ignore single. So clear changed
    }

    // *** light sensor
    auto t = time(nullptr);
    uint8_t tf = t % 60;
    if (tflag != tf)
    { // a new time
        tflag = tf;
        // light sensor installed at first slave
        lux = vsSlaves[0]->lux;
        auto lastLs = dbncLightSnsr.Value();
        if ((vsSlaves[0]->lightSensorFault & 1) == 0 && lux > 0)
        {
            if (dbncLightSnsr.Value() != STATE3::S_0)
            {
                dbncLightSnsr.SetState(false);
            }
        }
        else
        {
            dbncLightSnsr.Check(vsSlaves[0]->lightSensorFault & 1);
        }
        if (dbncLightSnsr.changed)
        {
            dbncLightSnsr.changed = false;
            if (dbncLightSnsr.Value() == STATE3::S_1)
            {
                lightSnsrFault = STATE3::S_1;
                signErr.Push(signId, DEV::ERROR::SignLuminanceControllerFailure, true);
                db.GetUciAlarm().Push(signId, "Light sensor DISCONNECTED");
            }
            else if (dbncLightSnsr.Value() == STATE3::S_0 && lastLs == STATE3::S_1)
            {
                db.GetUciAlarm().Push(signId, "Light sensor CONNECTED");
            }
        }

        if (dbncLightSnsr.Value() == STATE3::S_0)
        {
            auto &prod = DbHelper::Instance().GetUciProd();
            dbnc18hours.Check(lux < prod.LightSensor18Hours());
            struct tm stm;
            localtime_r(&t, &stm);
            if (stm.tm_hour >= 11 && stm.tm_hour < 15)
            { // mid-day
                if (lasthour != stm.tm_hour && (lasthour < 11 || lasthour > 15))
                {
                    dbncMidday.ResetCnt();
                }
                dbncMidday.Check(lux < prod.LightSensorMidday());
            }
            if (stm.tm_hour < 3 || stm.tm_hour >= 23)
            { // mid-night
                if (lasthour != stm.tm_hour && (lasthour < 23 || lasthour > 3))
                {
                    dbncMidnight.ResetCnt();
                }
                dbncMidnight.Check(lux > prod.LightSensorMidnight());
            }
            lasthour = stm.tm_hour;
            if (dbnc18hours.Value() == STATE3::S_1 ||
                dbncMidday.Value() == STATE3::S_1 ||
                dbncMidnight.Value() == STATE3::S_1)
            { // any failed
                if (lightSnsrFault != STATE3::S_1)
                {
                    signErr.Push(signId, DEV::ERROR::SignLuminanceControllerFailure, true);
                    lightSnsrFault = STATE3::S_1;
                }
            }
            else if (dbnc18hours.Value() == STATE3::S_0 &&
                     dbncMidday.Value() == STATE3::S_0 &&
                     dbncMidnight.Value() == STATE3::S_0)
            { // all good
                if (lightSnsrFault != STATE3::S_0)
                {
                    signErr.Push(signId, DEV::ERROR::SignLuminanceControllerFailure, false);
                    lightSnsrFault = STATE3::S_0;
                }
            }
            char buf[64];
            if (dbnc18hours.changed)
            {
                dbnc18hours.changed = false;
                if (dbnc18hours.Value() == STATE3::S_1)
                {
                    sprintf(buf, "Lux < %d for 18 hours: 18-Hour Fault ONSET", prod.LightSensor18Hours());
                }
                else
                {
                    sprintf(buf, "Lux >= %d for 15 minutes: 18-Hour Fault CLEAR", prod.LightSensor18Hours());
                }
                db.GetUciAlarm().Push(signId, buf);
            }
            if (dbncMidday.changed)
            {
                dbncMidday.changed = false;
                if (dbncMidday.Value() == STATE3::S_1)
                {
                    sprintf(buf, "In 11:00-15:00, Lux < %d for 15 minutes: Midday Fault ONSET", prod.LightSensorMidday());
                }
                else
                {
                    sprintf(buf, "In 11:00-15:00, Lux >= %d for 15 minutes: MiddayFault CLEAR", prod.LightSensorMidday());
                }
                db.GetUciAlarm().Push(signId, buf);
            }
            if (dbncMidnight.changed)
            {
                dbncMidnight.changed = false;
                if (dbncMidnight.Value() == STATE3::S_1)
                {
                    sprintf(buf, "In 23:00-3:00, Lux >= %d for 15 minutes: Midnight Fault ONSET", prod.LightSensorMidnight());
                }
                else
                {
                    sprintf(buf, "In 23:00-3:00, Lux < %d for 15 minutes: Midnight Fault CLEAR", prod.LightSensorMidnight());
                }
                db.GetUciAlarm().Push(signId, buf);
            }
        }
    }
}

uint8_t *Sign::LedStatus(uint8_t *buf)
{
    uint8_t tiles = SignTiles();
    uint8_t tbytes = (tiles + 7) / 8;
    memset(buf, 0, tbytes);
    int bitOffset = 0;
    for (auto &s : vsSlaves)
    {
        for (int i = 0; i < s->numberOfTiles; i++)
        {
            uint8_t tile = 0;
            for (int j = 0; j < s->numberOfColours; j++)
            {
                if (*(s->numberOfFaultyLed + j * s->numberOfTiles + i) > 0)
                {
                    tile = 1;
                    break;
                }
            }
            if (tile != 0)
            {
                BitOffset::SetBit(buf, bitOffset);
            }
            bitOffset++;
        }
    }
    return buf + tbytes;
}

void Sign::DbncFault(Debounce &dbc, DEV::ERROR err, const char *info)
{
    if (dbc.changed)
    {
        dbc.changed = false;
        char buf[64];
        int len = 0;
        if (dbc.Value() == STATE3::S_1)
        {
            if (!signErr.IsSet(err))
            {
                signErr.Push(signId, err, true);
                len = sprintf(buf, "Sign%d %s ONSET", signId, DEV::GetStr(err));
            }
        }
        else
        {
            if (signErr.IsSet(err))
            {
                signErr.Push(signId, err, false);
                len = sprintf(buf, "Sign%d %s CLEAR", signId, DEV::GetStr(err));
            }
        }
        if (len > 0)
        {
            if (info != nullptr)
            {
                snprintf(buf + len, 63 - len, ": %s", info);
            }
            DbHelper::Instance().GetUciAlarm().Push(signId, buf);
        }
    }
}
