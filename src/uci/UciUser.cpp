#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdlib.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <module/SerialPort.h>
#include <uci/DbHelper.h>

using namespace Utils;
extern time_t GetTime(time_t *);

UciUser::UciUser()
{
}

UciUser::~UciUser()
{
    if (tz_AU != nullptr)
    {
        delete tz_AU;
    }
}

void UciUser::LoadConfig()
{
    Ldebug(">>> Loading 'user'");
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciUser";
    DEFAULT_FILE = "UciUser.def";
    SECTION = "user_cfg";
    UciProd &uciProd = DbHelper::Instance().GetUciProd();
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;

    deviceId = GetInt(uciSec, _DeviceId, 0, 255);
    broadcastId = GetInt(uciSec, _BroadcastId, 0, 255);
    if (deviceId == broadcastId)
    {
        throw std::invalid_argument(FmtException("UciUser.DeviceId(%d) should not be same as BroadcastId(%d)",
                                                 deviceId, broadcastId));
    }
    seedOffset = GetInt(uciSec, _SeedOffset, 0, 255);
    fan1OnTemp = GetInt(uciSec, _Fan1OnTemp, 0, 100);
    fan2OnTemp = GetInt(uciSec, _Fan2OnTemp, 0, 100);
    overTemp = GetInt(uciSec, _OverTemp, 0, 100);
    humidity = GetInt(uciSec, _Humidity, 0, 100);
    /*
    defaultFont = GetInt(uciSec, _DefaultFont, 1, MAX_FONT);
    if (!uciProd.IsFont(defaultFont))
    {
        throw std::invalid_argument(FmtException("UciUser::DefaultFont(%d) is not valid", defaultFont));
    }
    defaultColour = GetInt(uciSec, _DefaultColour, 1, MAX_MONOCOLOUR);
    */

    nightDimmingLevel = GetInt(uciSec, _NightDimmingLevel, 1, 8);
    dawnDimmingLevel = GetInt(uciSec, _DawnDimmingLevel, nightDimmingLevel + 1, 15);
    dayDimmingLevel = GetInt(uciSec, _DayDimmingLevel, dawnDimmingLevel + 1, 16);
    luxDayMin = GetInt(uciSec, _LuxDayMin, 1, 65535);
    luxNightMax = GetInt(uciSec, _LuxNightMax, 1, 65535);
    lux18HoursMin = GetInt(uciSec, _Lux18HoursMin, 1, 65535);

    lastFrmTime = GetInt(uciSec, _LastFrmTime, 1, 60);
    lockedFrm = GetInt(uciSec, _LockedFrm, 0, 255);
    lockedMsg = GetInt(uciSec, _LockedMsg, 0, 255);
    passwordOffset = GetInt(uciSec, _PasswordOffset, 0, 0xFFFF);
    displayTimeoutMin = GetInt(uciSec, _DisplayTimeout, 0, 10080);
    sessionTimeoutSec = GetInt(uciSec, _SessionTimeout, 60, 0xFFFF);
    svcPort = GetInt(uciSec, _SvcPort, 1024, 0xFFFF);
    webPort = GetInt(uciSec, _WebPort, 1024, 0xFFFF);
    if (svcPort == webPort)
    {
        throw std::invalid_argument(FmtException("UciUser.SvcPort(%d) should not be same as WebPort(%d)", svcPort, webPort));
    }
    multiLedFaultThreshold = GetInt(uciSec, _MultiLedFaultThreshold, 0, 0xFFFF);

    baudrate = GetInt(uciSec, _Baudrate, ALLOWEDBPS, STANDARDBPS_SIZE);

    str = GetStr(uciSec, _ShakehandsPassword);
    if (str == NULL)
    {
        strcpy(shakehandsPassword, "brightway");
    }
    else
    {
        int len = strlen(str);
        if (len > 10)
            len = 10;
        memcpy(shakehandsPassword, str, len);
        shakehandsPassword[len] = '\0';
    }

    for (int m = 0; m < extSw.size(); m++)
    {
        sprintf(cbuf, "%s%d", _ExtSw, m + 1); // ExtSw1-4
        str = GetStr(uciSec, cbuf);
        cnt = Cnvt::GetIntArray(str, 4, ibuf, 0, 65535);
        if (cnt == 4)
        {
            auto &ext = extSw.at(m);
            ext.dispTime = ibuf[0];
            ext.reserved = ibuf[1];
            ext.emergency = ibuf[2];
            ext.flashingOv = ibuf[3];
        }
        else
        {
            throw std::invalid_argument(FmtException("UciUser.%s error: %s", cbuf, str));
        }
    }

    str = GetStr(uciSec, _City);
    cityId = NUMBER_OF_TZ;
    if (str != NULL)
    {
        for (int cnt = 0; cnt < NUMBER_OF_TZ; cnt++)
        {
            if (strcasecmp(str, Tz_AU::tz_au[cnt].city) == 0)
            {
                cityId = cnt;
                break;
            }
        }
    }
    if (cnt == NUMBER_OF_TZ)
    {
        throw std::invalid_argument(FmtException("UciUser.City error:%s", str));
    }

    str = GetStr(uciSec, _DawnDusk);
    cnt = Cnvt::GetIntArray(str, 16, ibuf, 0, 59);
    if (cnt == 16)
    {
        for (cnt = 0; cnt < 16; cnt += 2)
        {
            if (ibuf[cnt] > 23)
            {
                throw std::invalid_argument("UciUser.DawnDusk Error: Hour>23");
            }
        }
        for (cnt = 0; cnt < 16; cnt++)
        {
            dawnDusk[cnt] = ibuf[cnt];
        }
    }
    else
    {
        throw std::invalid_argument("UciUser.DawnDusk Error: cnt!=16");
    }
    if (tz_AU != nullptr)
    {
        tz_AU->Init_Tz(cityId, str);
    }
    else
    {
        tz_AU = new Tz_AU(cityId, str);
    }

    setenv("TZ", tz_AU->GetTz(), 1);
    tzset();

    str = GetStr(uciSec, _Luminance);
    cnt = Cnvt::GetIntArray(str, 16, ibuf, 1, 65535);
    if (cnt == 16)
    {
        for (cnt = 0; cnt < 15; cnt++)
        {
            if (ibuf[cnt] >= ibuf[cnt + 1])
            {
                throw std::invalid_argument(FmtException("UciUser.Luminance Error: [%d]%d>[%d]%d",
                                                         cnt, ibuf[cnt], cnt + 1, ibuf[cnt + 1]));
            }
        }
        for (cnt = 0; cnt < 16; cnt++)
        {
            luminance[cnt] = ibuf[cnt];
        }
    }
    else
    {
        throw std::invalid_argument("UciUser.Luminance Error: cnt!=16");
    }

    int numberOfSigns = uciProd.NumberOfSigns();

    comPort = GetIndexFromStrz(uciSec, _ComPort, COM_NAME, COMPORT_SIZE);
    for (uint8_t i = 1; i <= numberOfSigns; i++)
    {
        if (comPort == uciProd.GetSignCfg(i).com_ip)
        {
            throw std::invalid_argument(FmtException("UciUser.%s: %s used by UciProd.Sign[%d]", _ComPort, COM_NAME[comPort], i));
        }
    }
    if (comPort == uciProd.MonitoringPort())
    {
        throw std::invalid_argument(FmtException("UciUser.%s: %s used by UciProd.MonitoringPort", _ComPort, COM_NAME[comPort]));
    }

    Close();
    Dump();
}

void UciUser::LoadFactoryDefault()
{
    char def[256];
    char uci[256];
    snprintf(uci, 255, "%s/%s", PATH, PACKAGE);
    snprintf(def, 255, "%s/%s", PATH, DEFAULT_FILE);
    Exec::CopyFile(def, uci);
    OpenSaveClose(SECTION, _DeviceId, DeviceId());
    OpenSaveClose(SECTION, _BroadcastId, BroadcastId());
    LoadConfig();
}

void UciUser::Dump()
{
    PrintDash('<');
    printf("%s/%s.%s\n", PATH, PACKAGE, SECTION);
    PrintOption_d(_DeviceId, DeviceId());
    PrintOption_d(_BroadcastId, BroadcastId());
    PrintOption_2x(_SeedOffset, SeedOffset());
    PrintOption_4x(_PasswordOffset, PasswordOffset());
    PrintOption_d(_SvcPort, SvcPort());
    PrintOption_d(_WebPort, WebPort());
    PrintOption_d(_Baudrate, Baudrate());
    PrintOption_d(_OverTemp, OverTemp());
    PrintOption_d(_Fan1OnTemp, Fan1OnTemp());
    PrintOption_d(_Fan2OnTemp, Fan2OnTemp());
    PrintOption_d(_Humidity, Humidity());
    PrintOption_d(_SessionTimeout, SessionTimeoutSec());
    PrintOption_d(_DisplayTimeout, DisplayTimeoutMin());
    //PrintOption_d(_DefaultFont, DefaultFont());
    //PrintOption_d(_DefaultColour, DefaultColour());
    PrintOption_d(_MultiLedFaultThreshold, MultiLedFaultThreshold());
    PrintOption_d(_LockedFrm, LockedFrm());
    PrintOption_d(_LockedMsg, LockedMsg());
    PrintOption_d(_LastFrmTime, LastFrmTime());

    PrintOption_str(_City, City());
    PrintOption_str(_ComPort, COM_NAME[ComPort()]);

    char buf[256];

    for (int i = 0; i < extSw.size(); i++)
    {
        PrintExtSw(i, buf);
        printf("\t%s%d \t'%s'\n", _ExtSw, i + 1, buf);
    }

    PrintOption_d(_LuxDayMin, LuxDayMin());
    PrintOption_d(_LuxNightMax, LuxNightMax());
    PrintOption_d(_Lux18HoursMin, Lux18HoursMin());
    PrintOption_d(_NightDimmingLevel, NightDimmingLevel());
    PrintOption_d(_DawnDimmingLevel, DawnDimmingLevel());
    PrintOption_d(_DayDimmingLevel, DayDimmingLevel());

    PrintLuminance(buf);
    printf("\t%s \t'%s'\n", _Luminance, buf);

    PrintDawnDusk(buf);
    printf("\t%s \t'%s'\n", _DawnDusk, buf);

    PrintDash('>');
}

void UciUser::PrintExtSw(int i, char *buf)
{
    auto &exswcfg = ExtSwCfgX(i);
    sprintf(buf, "%d,%d,%d,%d",
            exswcfg.dispTime, exswcfg.reserved, exswcfg.emergency, exswcfg.flashingOv);
}

void UciUser::PrintDawnDusk(char *buf)
{
    uint8_t *p = DawnDusk();
    int len = 0;
    for (int i = 0; i < 8; i++)
    {
        len += sprintf(buf + len, (i == 0) ? "%u:%02u" : ",%u:%02u", *p, *(p + 1));
        p += 2;
    }
}

void UciUser::PrintLuminance(char *buf)
{
    int len = 0;
    for (int i = 0; i < 16; i++)
    {
        len += sprintf(buf + len, (i == 0) ? "%u" : ",%u", luminance[i]);
    }
}

uint8_t UciUser::GetLuxLevel(int lux)
{
    if (lux < 0)
    {
        switch (tz_AU->GetTwilightStatus(GetTime(nullptr)))
        {
        case Tz_AU::TwilightStatus::TW_ST_NIGHT:
            return nightDimmingLevel;
        case Tz_AU::TwilightStatus::TW_ST_DAY:
            return dayDimmingLevel;
        default:
            return dawnDimmingLevel;
        }
    }
    else
    {
        for (int i = 0; i < 16; i++)
        {
            if (lux <= luminance[i])
            {
                return i + 1;
            }
        }
        return 16;
    }
}

/// --------setter--------

void UciUser::BroadcastId(uint8_t v)
{
    if (broadcastId != v)
    {
        broadcastId = v;
        OpenSaveClose(SECTION, _BroadcastId, v);
    }
}

void UciUser::DeviceId(uint8_t v)
{
    if (deviceId != v)
    {
        deviceId = v;
        OpenSaveClose(SECTION, _DeviceId, v);
    }
}

void UciUser::SeedOffset(uint8_t v)
{
    if (seedOffset != v)
    {
        seedOffset = v;
        char buf[8];
        sprintf(buf, "0x%02X", v);
        OpenSaveClose(SECTION, _SeedOffset, buf);
    }
}

void UciUser::Fan1OnTemp(uint8_t v)
{
    if (fan1OnTemp != v)
    {
        fan1OnTemp = v;
        OpenSaveClose(SECTION, _Fan1OnTemp, v);
    }
}

void UciUser::Fan2OnTemp(uint8_t v)
{
    if (fan2OnTemp != v)
    {
        fan2OnTemp = v;
        OpenSaveClose(SECTION, _Fan2OnTemp, v);
    }
}

void UciUser::OverTemp(uint8_t v)
{
    if (overTemp != v)
    {
        overTemp = v;
        OpenSaveClose(SECTION, _OverTemp, v);
    }
}

void UciUser::Humidity(uint8_t v)
{
    if (humidity != v)
    {
        humidity = v;
        OpenSaveClose(SECTION, _Humidity, v);
    }
}

/*
void UciUser::DefaultFont(uint8_t v)
{
    if (defaultFont != v)
    {
        defaultFont = v;
        OpenSaveClose(SECTION, _DefaultFont, v);
    }
}

void UciUser::DefaultColour(uint8_t v)
{
    if (defaultColour != v)
    {
        defaultColour = v;
        OpenSaveClose(SECTION, _DefaultColour, v);
    }
}
*/
void UciUser::LockedFrm(uint8_t v)
{
    if (lockedFrm != v)
    {
        lockedFrm = v;
        OpenSaveClose(SECTION, _LockedFrm, v);
    }
}

void UciUser::LockedMsg(uint8_t v)
{
    if (lockedMsg != v)
    {
        lockedMsg = v;
        OpenSaveClose(SECTION, _LockedMsg, v);
    }
}

void UciUser::LastFrmTime(uint8_t v)
{
    if (lastFrmTime != v)
    {
        lastFrmTime = v;
        OpenSaveClose(SECTION, _LastFrmTime, v);
    }
}

void UciUser::ComPort(uint8_t v)
{
    if (comPort != v)
    {
        comPort = v;
        OpenSaveClose(SECTION, _ComPort, COM_NAME[comPort]);
    }
}

void UciUser::MultiLedFaultThreshold(uint16_t v)
{
    if (multiLedFaultThreshold != v)
    {
        multiLedFaultThreshold = v;
        OpenSaveClose(SECTION, _MultiLedFaultThreshold, v);
    }
}

void UciUser::PasswordOffset(uint16_t v)
{
    if (passwordOffset != v)
    {
        passwordOffset = v;
        char buf[8];
        sprintf(buf, "0x%04X", v);
        OpenSaveClose(SECTION, _PasswordOffset, buf);
    }
}

void UciUser::SessionTimeoutSec(uint16_t v)
{
    if (sessionTimeoutSec != v)
    {
        sessionTimeoutSec = v;
        OpenSaveClose(SECTION, _SessionTimeout, v);
    }
}

void UciUser::DisplayTimeoutMin(uint16_t v)
{
    if (displayTimeoutMin != v)
    {
        displayTimeoutMin = v;
        OpenSaveClose(SECTION, _DisplayTimeout, v);
    }
}

void UciUser::SvcPort(uint16_t v)
{
    if (svcPort != v)
    {
        svcPort = v;
        OpenSaveClose(SECTION, _SvcPort, v);
    }
}

void UciUser::WebPort(uint16_t v)
{
    if (webPort != v)
    {
        webPort = v;
        OpenSaveClose(SECTION, _WebPort, v);
    }
}

void UciUser::Baudrate(int v)
{
    if (baudrate != v)
    {
        baudrate = v;
        OpenSaveClose(SECTION, _Baudrate, v);
    }
}

void UciUser::CityId(uint8_t v)
{
    if (cityId != v)
    {
        cityId = v;
        OpenSaveClose(SECTION, _City, Tz_AU::tz_au[cityId].city);
        char buf[1024];
        PrintDawnDusk(buf);
        tz_AU->Init_Tz(cityId, buf);
        setenv("TZ", tz_AU->GetTz(), 1);
        tzset();
    }
}

const char *UciUser::City()
{
    return Tz_AU::tz_au[cityId].city;
}

void UciUser::DawnDusk(uint8_t *p)
{
    if (memcmp(p, dawnDusk, sizeof(dawnDusk)) != 0)
    {
        memcpy(dawnDusk, p, sizeof(dawnDusk));
        char buf[1024];
        PrintDawnDusk(buf);
        OpenSaveClose(SECTION, _DawnDusk, buf);
        tz_AU->Init_Tz(cityId, buf);
    }
}

void UciUser::Luminance(uint16_t *p)
{
    if (memcmp(luminance, p, sizeof(luminance)) != 0)
    {
        memcpy(luminance, p, sizeof(luminance));
        char buf[1024];
        PrintLuminance(buf);
        OpenSaveClose(SECTION, _Luminance, buf);
    }
}

void UciUser::ExtSwCfgX(int i, ExtSw &cfg)
{
    if (i >= 0 && i < extSw.size() && !extSw.at(i).Equal(cfg))
    {
        memcpy(&extSw.at(i), &cfg, sizeof(ExtSw));
        char op[32];
        strcpy(op, _ExtSw);
        op[5] = i + '1';
        char buf[1024];
        PrintExtSw(i, buf);
        OpenSaveClose(SECTION, op, buf);
    }
}

void UciUser::ShakehandsPassword(const char *shake)
{
    for (int i = 0; i < 10; i++)
    {
        shakehandsPassword[i] = shake[i];
        if (shake[i] == '\0')
        {
            return;
        }
    }
    shakehandsPassword[10] = '\0';
}

void UciUser::NightDimmingLevel(uint8_t v)
{
    if (nightDimmingLevel != v)
    {
        nightDimmingLevel = v;
        OpenSaveClose(SECTION, _NightDimmingLevel, v);
    }
}

void UciUser::DayDimmingLevel(uint8_t v)
{
    if (dayDimmingLevel != v)
    {
        dayDimmingLevel = v;
        OpenSaveClose(SECTION, _DayDimmingLevel, v);
    }
}

void UciUser::DawnDimmingLevel(uint8_t v)
{
    if (dawnDimmingLevel != v)
    {
        dawnDimmingLevel = v;
        OpenSaveClose(SECTION, _DawnDimmingLevel, v);
    }
}

void UciUser::LuxDayMin(uint16_t v)
{
    if (luxDayMin != v)
    {
        luxDayMin = v;
        OpenSaveClose(SECTION, _LuxDayMin, v);
    }
}

void UciUser::LuxNightMax(uint16_t v)
{
    if (luxNightMax != v)
    {
        luxNightMax = v;
        OpenSaveClose(SECTION, _LuxNightMax, v);
    }
}

void UciUser::Lux18HoursMin(uint16_t v)
{
    if (lux18HoursMin != v)
    {
        lux18HoursMin = v;
        OpenSaveClose(SECTION, _Lux18HoursMin, v);
    }
}
