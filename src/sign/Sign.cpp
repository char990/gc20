#include <cstring>
#include <sign/Sign.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
extern time_t GetTime(time_t *);

Sign::Sign(uint8_t id)
    : signId(id), frameImages(7)
{
    auto &ucihw = DbHelper::Instance().GetUciHardware();
    chainFault.SetCNT(ucihw.DriverFaultDebounce());
    multiLedFault.SetCNT(ucihw.MLedFaultDebounce());
    singleLedFault.SetCNT(ucihw.SLedFaultDebounce());
    selftestFault.SetCNT(ucihw.SelftestDebounce());
    voltageFault.SetCNT(ucihw.SlaveVoltageDebounce());
    lanternFault.SetCNT(ucihw.LanternFaultDebounce());
    overtempFault.SetCNT(ucihw.OverTempDebounce());

    lsConnectionFault.SetCNT(ucihw.LightSensorFaultDebounce());
    // light sensor fault debounce 1 minute in slave. If true_cnt - ucihw.LightSensorFaultDebounce() is 120, report after 3 minutes
    // If receive LightsensorFault=0 && Lux>0, clear lsConnectionFault immediately
    ls18hoursFault.SetCNT(18 * 60 * 60, 15 * 60);
    lsMidnightFault.SetCNT(15 * 60);
    lsMiddayFault.SetCNT(15 * 60);

    for (int i = 0; i < frameImages.size(); i++)
    {
        frameImages[i].SetId(id, i);
    }
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
    auto t1 = GetTime(nullptr);
    auto t2 = t1 - timeSt;
    if (t2 == 0)
    {
        return;
    }
    else if (t2 < 0 || t2 > 10)
    {
        t2 = 1;
    }
    timeSt = t1;
    // ----------------------Check status
    // single & multiLed bits ignored. checked in ext_st
    // over-temperature bits ignored. checked in ext_st
    char buf[STRLOG_SIZE];
    int len;
    uint8_t check;

    // lanterns installed at first&last slaves
    buf[0] = '\0';
    check = (vsSlaves.size() == 1) ? (vsSlaves[0]->lanternFan & 0x0F) : // only one slave
                ((vsSlaves[0]->lanternFan & 0x03) | ((vsSlaves[vsSlaves.size() - 1]->lanternFan & 0x03) << 2));
    lanternFault.Check(check > 0, t2);
    if (check > 0)
    {
        sprintf(buf, "LanternFault=0x%02X", check);
    }
    DbncFault(lanternFault, DEV::ERROR::ConspicuityDeviceFailure, buf);

    auto &ucihw = DbHelper::Instance().GetUciHardware();

    // Chain
    check = 0;
    buf[0] = '\0';
    len = 0;
    for (auto &s : vsSlaves)
    {
        auto cf = s->mledchainFault & 0x0F;
        check |= cf;
        if (cf)
        {
            len += snprintf(buf, STRLOG_SIZE - len - 1, " [%d]=0x%02X", s->SlaveId(), cf);
        }
    }
    chainFault.Check(check > 0, t2);
    DbncFault(chainFault, DEV::ERROR::SignDisplayDriverFailure, buf);

    // Selftest
    check = 0;
    buf[0] = '\0';
    len = 0;
    for (auto &s : vsSlaves)
    {
        if (s->selfTest != 0)
        {
            check |= s->selfTest;
            if (len == 0)
            {
                len = sprintf(buf, "Selftest:");
            }
            len += snprintf(buf + len, STRLOG_SIZE - 1 - len, " [%d]=0x%02X", s->SlaveId(), s->selfTest);
        }
    }
    selftestFault.Check(check > 0, t2);
    DbncFault(selftestFault, DEV::ERROR::UnderLocalControl, buf);

    RefreshFatalError();
}

void Sign::RefreshSlaveStatusAtExtSt()
{
    for (auto &s : vsSlaves)
    {
        if (s->GetRxExtSt() == 0)
        {
            return;
        }
    }
    auto t1 = GetTime(nullptr);
    auto t2 = t1 - timeExtSt;
    if (t2 == 0)
    {
        return;
    }
    else if (t2 < 0 || t2 > 10)
    {
        t2 = 1;
    }
    timeExtSt = t1;
    // ----------------------Check status
    // single & multiLed bits ignored. checked in ext_st
    // over-temperature bits ignored. checked in ext_st
    char buf[64];
    auto &db = DbHelper::Instance();
    auto &ucihw = db.GetUciHardware();
    auto &usercfg = db.GetUciUserCfg();

    // ----------------------Check ext-status
    uint16_t minvoltage = 0xFFFF, maxvoltage = 0; // mV
    uint32_t v = 0;
    uint8_t vcnt = 0;
    int16_t temperature = 0; // 0.1'C
    uint32_t faultLeds = 0;
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
        for (auto x : s->faultyLedPerColour)
        {
            faultLeds += x;
        }
    }
    // *** voltage
    if (minvoltage < ucihw.SlaveVoltageLow())
    {
        voltageFault.Check(true, t2);
        voltage = minvoltage;
    }
    else if (maxvoltage > ucihw.SlaveVoltageHigh())
    {
        voltageFault.Check(true, t2);
        voltage = maxvoltage;
    }
    else
    {
        voltageFault.Check(false, t2);
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
    auto ot = usercfg.OverTemp();
    if (curTemp > ot)
    {
        overtempFault.Check(true, t2);
        if (!signErr.IsSet(DEV::ERROR::OverTemperatureAlarm) && overtempFault.IsHigh())
        {
            signErr.Push(signId, DEV::ERROR::OverTemperatureAlarm, true);
            DbHelper::Instance().GetUciAlarm().Push(signId, "Sign[%d[] OverTemperatureAlarm ONSET: %d'C", signId, curTemp);
            overtempFault.ClearEdge();
        }
    }
    else if (curTemp < (ot - 3))
    {
        overtempFault.Check(false, t2);
        if (signErr.IsSet(DEV::ERROR::OverTemperatureAlarm) && overtempFault.IsLow())
        {
            signErr.Push(signId, DEV::ERROR::OverTemperatureAlarm, false);
            DbHelper::Instance().GetUciAlarm().Push(signId, "Sign[%d] OverTemperatureAlarm CLEAR: %d'C", signId, curTemp);
            overtempFault.ClearEdge();
        }
    }

    // *** single/multi led
    if (chainFault.IsLow())
    {
        if (faultLeds > 65535)
        {
            faultLeds = 65535;
        }

        sprintf(buf, "%d LEDs", faultLeds);
        multiLedFault.Check(faultLeds >= usercfg.MultiLedFaultThreshold(), t2);
        if (multiLedFault.IsRising() || multiLedFault.IsFalling())
        {
            faultLedCnt = faultLeds;
        }
        DbncFault(multiLedFault, DEV::ERROR::SignMultiLedFailure, buf);
        if (multiLedFault.IsLow())
        {
            singleLedFault.Check(faultLeds > 0, t2);
            if (singleLedFault.IsRising() || singleLedFault.IsFalling())
            {
                faultLedCnt = faultLeds;
            }
            DbncFault(singleLedFault, DEV::ERROR::SignSingleLedFailure, buf);
        }
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
        lsConnectionFault.Check(vsSlaves[0]->lightSensorFault & 1, t2);
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
        ls18hoursFault.Check(lux < usercfg.Lux18HoursMin(), t2);
        struct tm stm;
        localtime_r(&t, &stm);
        if (stm.tm_hour >= 11 && stm.tm_hour < 15)
        { // mid-day
            if (lasthour != stm.tm_hour && (lasthour < 11 || lasthour > 15))
            {
                lsMiddayFault.ResetCnt();
            }
            lsMiddayFault.Check(lux < usercfg.LuxDayMin(), t2);
        }
        if (stm.tm_hour < 3 || stm.tm_hour >= 23)
        { // mid-night
            if (lasthour != stm.tm_hour && (lasthour < 23 || lasthour > 3))
            {
                lsMidnightFault.ResetCnt();
            }
            lsMidnightFault.Check(lux > usercfg.LuxNightMax(), t2);
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
                db.GetUciAlarm().Push(signId, "Lux(%d)<%d for 18-hour: LS-18hoursFault ONSET", lux, usercfg.Lux18HoursMin());
            }
            else
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)>=%d for 18-hour: LS-18hoursFault CLEAR", lux, usercfg.Lux18HoursMin());
            }
        }
        if (lsMiddayFault.HasEdge())
        {
            lsMiddayFault.ClearEdge();
            if (lsMiddayFault.IsHigh())
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)<%d(15-min in 11am-3pm): LS-MiddayFault ONSET", lux, usercfg.LuxDayMin());
            }
            else
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)>=%d(15-min in 11am-3pm): LS-MiddayFault CLEAR", lux, usercfg.LuxDayMin());
            }
        }
        if (lsMidnightFault.HasEdge())
        {
            lsMidnightFault.ClearEdge();
            if (lsMidnightFault.IsHigh())
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)>=%d(15-min in 23pm-3am): LS-MidnightFault ONSET", lux, usercfg.LuxNightMax());
            }
            else
            {
                db.GetUciAlarm().Push(signId, "Lux(%d)<%d(15-min in 23pm-3am): LS-MidnightFault CLEAR", lux, usercfg.LuxNightMax());
            }
        }
    }
}

void Sign::RefreshFatalError()
{
    char buf[6]{0};
    char *p = buf;
    if (chainFault.IsHigh())
    {
        *p++ = 'C';
    }
    if (multiLedFault.IsHigh())
    {
        *p++ = 'L';
    }
    if (selftestFault.IsHigh())
    {
        *p++ = 'S';
    }
    if (voltageFault.IsHigh())
    {
        *p++ = 'V';
    }
    if (overtempFault.IsHigh())
    {
        *p++ = 'T';
    }
    if (p != buf)
    {
        fatalError.Set();
        if (fatalError.IsRising())
        {
            Ldebug("Sign[%d]:fatalError Set:(%s)", signId, buf);
        }
    }
    else
    {
        fatalError.Clr();
        if (fatalError.IsFalling() && (fatalError.PreV() != STATE5::S5_NA))
        {
            Ldebug("Sign[%d]:fatalError Clr", signId);
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
                if (s->numberOfFaultyLed.at(j * s->numberOfTiles + i) > 0)
                {
                    BitOffset::Set07Bit(buf, bitOffset);
                    break;
                }
            }
        }
    }
    return buf + tbytes;
}

void Sign::DbncFault(Debounce &dbc, DEV::ERROR err, const char *info)
{
    char buf[STRLOG_SIZE];
    int len = 0;
    if (dbc.IsRising())
    {
        if (!signErr.IsSet(err))
        {
            signErr.Push(signId, err, true);
            len = snprintf(buf, STRLOG_SIZE - 1, "Sign[%d] %s ONSET", signId, DEV::ToStr(err));
        }
    }
    else if (dbc.IsFalling())
    {
        if (signErr.IsSet(err))
        {
            signErr.Push(signId, err, false);
            len = snprintf(buf, STRLOG_SIZE - 1, "Sign[%d] %s CLEAR", signId, DEV::ToStr(err));
        }
    }
    dbc.ClearEdge();
    if (len > 0)
    {
        if (info != nullptr && strlen(info) > 0)
        {
            snprintf(buf + len, STRLOG_SIZE - 1 - len, ": %s", info);
        }
        DbHelper::Instance().GetUciAlarm().Push(signId, buf);
    }
}

void Sign::RefreshDevErr(DEV::ERROR err)
{
    bool result = false;
    int cnt0 = 0;
    if (err == DEV::ERROR::InternalCommunicationsFailure)
    {
        for (auto s : vsSlaves)
        {
            if (s->isOffline == 1)
            {
                result = true;
                break;
            }
            else if (s->isOffline == 0)
            {
                cnt0++;
            }
        }
        if (result != true /*None is true*/ && cnt0 != vsSlaves.size() /*Not all 0*/)
        {
            return;
        }
    }
    else if (err == DEV::ERROR::MainProcessorCommunicationsError)
    {
        for (auto s : vsSlaves)
        {
            if (s->isCNCrcErr == 1)
            {
                result = true;
                break;
            }
            else if (s->isCNCrcErr == 0)
            {
                cnt0++;
            }
        }
        if (result != true /*None is true*/ && cnt0 != vsSlaves.size() /*Not all 0*/)
        {
            return;
        }
    }
    if (signErr.IsSet(err) != result)
    {
        SignErr(err, result);
    }
}

const char *Sign::GetImageBase64()
{
    return frameImages[(reportFrm == 0) ? 0 : slaveFrameId].Save2Base64().data();
}
