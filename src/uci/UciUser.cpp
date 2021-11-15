#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdlib.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <module/SerialPort.h>
#include <uci/DbHelper.h>

using namespace Utils;

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
    PrintDbg(DBG_LOG, ">>> Loading 'user'\n");
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
        MyThrow("UciUser::DeviceId(%d) should not be same as BroadcastId(%d)", deviceId, broadcastId);
    }
    seedOffset = GetInt(uciSec, _SeedOffset, 0, 255);
    fan1OnTemp = GetInt(uciSec, _Fan1OnTemp, 0, 100);
    fan2OnTemp = GetInt(uciSec, _Fan2OnTemp, 0, 100);
    overTemp = GetInt(uciSec, _OverTemp, 0, 100);
    humidity = GetInt(uciSec, _Humidity, 0, 100);
    defaultFont = GetInt(uciSec, _DefaultFont, 1, MAX_FONT);
    if (!uciProd.IsFont(defaultFont))
    {
        MyThrow("UciUser::DefaultFont(%d) is not valid", defaultFont);
    }
    defaultColour = GetInt(uciSec, _DefaultColour, 1, MAX_MONOCOLOUR);
    if (!uciProd.IsTxtFrmColourValid(defaultColour))
    {
        MyThrow("UciUser::DefaultColour(%d) is not valid in TextFrameColour", defaultColour);
    }
    if (!uciProd.IsGfxFrmColourValid(defaultColour))
    {
        MyThrow("UciUser::DefaultColour(%d) is not valid in GfxFrameColour", defaultColour);
    }
    if (!uciProd.IsHrgFrmColourValid(defaultColour))
    {
        MyThrow("UciUser::DefaultColour(%d) is not valid in HrgFrameColour", defaultColour);
    }

    lastFrmOn = GetInt(uciSec, _LastFrmOn, 1, 60);
    lockedFrm = GetInt(uciSec, _LockedFrm, 0, 255);
    lockedMsg = GetInt(uciSec, _LockedMsg, 0, 255);
    passwordOffset = GetInt(uciSec, _PasswordOffset, 0, 0xFFFF);
    displayTimeout = GetInt(uciSec, _DisplayTimeout, 0, 0xFFFF);
    sessionTimeout = GetInt(uciSec, _SessionTimeout, 1, 3600);
    svcPort = GetInt(uciSec, _SvcPort, 1024, 0xFFFF);
    webPort = GetInt(uciSec, _WebPort, 1024, 0xFFFF);
    if (svcPort == webPort)
    {
        MyThrow("UciUser::SvcPort(%d) should not be same as WebPort(%d)", svcPort, webPort);
    }
    multiLedFaultThreshold = GetInt(uciSec, _MultiLedFaultThreshold, 0, 0xFFFF);

    baudrate = GetInt(uciSec, _Baudrate, ALLOWEDBPS[0], ALLOWEDBPS[STANDARDBPS_SIZE - 1]);
    {
        for (cnt = 0; cnt < STANDARDBPS_SIZE; cnt++)
        {
            if (baudrate == ALLOWEDBPS[cnt])
                break;
        }
        if (cnt == STANDARDBPS_SIZE)
        {
            MyThrow("UciUser::Unknown Baudrate");
        }
    }

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

    for (int m = 0; m < 3; m++)
    {
        sprintf(cbuf, "%s%d", _ExtSw, m + 1); // ExtSw_
        str = GetStr(uciSec, cbuf);
        cnt = Cnvt::GetIntArray(str, 4, ibuf, 0, 65535);
        if (cnt == 4)
        {
            extSw[m].dispTime = ibuf[0];
            extSw[m].reserved = ibuf[1];
            extSw[m].emergency = ibuf[2];
            extSw[m].flashingOv = ibuf[3];
        }
        else
        {
            MyThrow("UciUser::%s error: %s", cbuf, str);
        }
    }

    str = GetStr(uciSec, _TZ);
    tz = NUMBER_OF_TZ;
    if (str != NULL)
    {
        for (int cnt = 0; cnt < NUMBER_OF_TZ; cnt++)
        {
            if (strcasecmp(str, Tz_AU::tz_au[cnt].city) == 0)
            {
                tz = cnt;
                break;
            }
        }
    }
    if (cnt == NUMBER_OF_TZ)
    {
        MyThrow("UciUser::TZ error:%s", str);
    }

    str = GetStr(uciSec, _DawnDusk);
    cnt = Cnvt::GetIntArray(str, 16, ibuf, 0, 59);
    if (cnt == 16)
    {
        for (cnt = 0; cnt < 16; cnt += 2)
        {
            if (ibuf[cnt] > 23)
            {
                MyThrow("UciUser::DawnDusk Error: Hour>23");
            }
        }
        for (cnt = 0; cnt < 16; cnt++)
        {
            dawnDusk[cnt] = ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciUser::DawnDusk Error: cnt!=16");
    }
    if (tz_AU != nullptr)
    {
        tz_AU->Init_Tz(tz, str);
    }
    else
    {
        tz_AU = new Tz_AU(tz, str);
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
                MyThrow("UciUser::Luminance Error: [%d]%d>[%d]%d", cnt, ibuf[cnt], cnt + 1, ibuf[cnt + 1]);
            }
        }
        for (cnt = 0; cnt < 16; cnt++)
        {
            luminance[cnt] = ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciUser::Luminance Error: cnt!=16");
    }

    int numberOfSigns = uciProd.NumberOfSigns();

    const char *COM_NAME[COMPORT_SIZE];
    for (int i = 0; i < COMPORT_SIZE; i++)
    {
        COM_NAME[i] = gSpConfig[i].name;
    }
    str = GetStr(uciSec, _ComPort);
    comPort = GetIntFromStrz(uciSec, _ComPort, COM_NAME, COMPORT_SIZE);
    for (uint8_t i = 1; i <= numberOfSigns; i++)
    {
        if (comPort == uciProd.SignPort(i)->com_ip)
        {
            MyThrow("UciUser::%s: %s assigned to Sign%d", _ComPort, COM_NAME[i - 1], i);
        }
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
    PrintDash();
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
    PrintOption_d(_SessionTimeout, SessionTimeout());
    PrintOption_d(_DisplayTimeout, DisplayTimeout());
    PrintOption_d(_DefaultFont, DefaultFont());
    PrintOption_d(_DefaultColour, DefaultColour());
    PrintOption_d(_MultiLedFaultThreshold, MultiLedFaultThreshold());
    PrintOption_d(_LockedFrm, LockedFrm());
    PrintOption_d(_LockedMsg, LockedMsg());
    PrintOption_d(_LastFrmOn, LastFrmOn());

    PrintOption_str(_TZ, TZ());
    PrintOption_str(_ComPort, gSpConfig[ComPort()].name);

    char buf[256];

    for (int i = 0; i < 3; i++)
    {
        PrintExtSw(i, buf);
        printf("\t%s%d \t'%s'\n", _ExtSw, i + 1, buf);
    }

    PrintLuminance(buf);
    printf("\t%s \t'%s'\n", _Luminance, buf);

    PrintDawnDusk(buf);
    printf("\t%s \t'%s'\n", _DawnDusk, buf);
}

void UciUser::PrintExtSw(int i, char *buf)
{
    ExtSw *exswcfg = ExtSwCfgX(i);
    sprintf(buf, "%d,%d,%d,%d",
            exswcfg->dispTime, exswcfg->reserved, exswcfg->emergency, exswcfg->flashingOv);
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
        switch (tz_AU->GetTwilightStatus(time(nullptr)))
        {
        case Tz_AU::TwilightStatus::TW_ST_NIGHT:
            return 1;
        case Tz_AU::TwilightStatus::TW_ST_DAY:
            return 16;
        default:
            return 8;
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
        sprintf(buf,"0x%02X",v);
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

void UciUser::LastFrmOn(uint8_t v)
{
    if (lastFrmOn != v)
    {
        lastFrmOn = v;
        OpenSaveClose(SECTION, _LastFrmOn, v);
    }
}

void UciUser::ComPort(uint8_t v)
{
    if (comPort != v)
    {
        comPort = v;
        OpenSaveClose(SECTION, _ComPort, gSpConfig[comPort].name);
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
        sprintf(buf,"0x%04X",v);
        OpenSaveClose(SECTION, _PasswordOffset, buf);
    }
}

void UciUser::SessionTimeout(uint16_t v)
{
    if (sessionTimeout != v)
    {
        sessionTimeout = v;
        OpenSaveClose(SECTION, _SessionTimeout, v);
    }
}

void UciUser::DisplayTimeout(uint16_t v)
{
    if (displayTimeout != v)
    {
        displayTimeout = v;
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

void UciUser::Tz(uint8_t v)
{
    if (tz != v)
    {
        tz = v;
        OpenSaveClose(SECTION, _TZ, Tz_AU::tz_au[tz].city);
    }
}

const char *UciUser::TZ()
{
    return Tz_AU::tz_au[tz].city;
}

void UciUser::DawnDusk(uint8_t *p)
{
    if (memcmp(p, dawnDusk, sizeof(dawnDusk)) != 0)
    {
        memcpy(dawnDusk, p, sizeof(dawnDusk));
        char buf[1024];
        PrintDawnDusk(buf);
        OpenSaveClose(SECTION, _DawnDusk, buf);
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

void UciUser::ExtSwCfgX(int i, ExtSw *cfg)
{
    if (i > 5)
        return;
    if (i >= 3 && i <= 5)
    {
        i -= 3;
    }
    if (!extSw[i].Equal(cfg))
    {
        memcpy(&extSw[i], cfg, sizeof(ExtSw));
        char op[32];
        strcpy(op, _ExtSw);
        op[5] = i + '1';
        char buf[1024];
        PrintExtSw(i, buf);
        OpenSaveClose(SECTION, op, buf);
    }
}


void UciUser::ShakehandsPassword(const char * shake)
{
    for(int i=0;i<10;i++)
    {
        shakehandsPassword[i]=shake[i];
        if(shake[i]=='\0')
        {
            return;
        }
    }
    shakehandsPassword[10]='\0';
}
