#include <cstdio>
#include <cstring>
#include <uci/UciUser.h>

using namespace Utils;

const char *ComPort[7] = {
    "MODEM",
    "COM1",
    "COM2",
    "COM3",
    "COM4",
    "COM5",
    "COM6",
};

#define SECTION_NAME "user_cfg"

#define _DeviceId "DeviceId"
#define _BroadcastAddr "BroadcastAddr"
#define _SeedOffset "SeedOffset"
#define _Fan1OnTemp "Fan1OnTemp"
#define _Fan2OnTemp "Fan2OnTemp"
#define _OverTemp "OverTemp"
#define _Humidity "Humidity"
#define _DefaultFont "DefaultFont"
#define _DefaultColour "DefaultColour"
#define _LockedFrm "LockedFrm"
#define _LockedMsg "LockedMsg"
#define _PasswordOffset "PasswordOffset"
#define _SessionTimeout "SessionTimeout"
#define _DisplayTimeout "DisplayTimeout"
#define _SvcPort "SvcPort"
#define _WebPort "WebPort"
#define _MultiLedFaultThreshold "MultiLedFaultThreshold"
#define _BaudrateTMC "BaudrateTMC"

#define _TZ "TZ"
#define _ShakehandsPassword "ShakehandsPassword"
#define _Luminance "Luminance"
#define _DawnDusk "DawnDusk"
#define _ComportTMC "ComportTMC"

#define ExtSw_Cfg "ExtSw_Cfg"

//GrpSign * grpSign;

UciUser::UciUser()
{
    PATH = "/etc/config";
    PACKAGE = "goblin_user";
    isChanged = false;
    grpSign = nullptr;
}

UciUser::~UciUser()
{
    if (grpSign != nullptr)
    {
        delete[] grpSign;
    }
}

void UciUser::LoadConfig()
{
    Open();
    sec = GetSection(SECTION_NAME);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;

    deviceId = GetInt(sec, _DeviceId, 1, 0, 255);
    broadcastAddr = GetInt(sec, _BroadcastAddr, 0, 0, 255);
    if (deviceId == broadcastAddr)
    {
        MyThrow("DeviceId(%d) should be not same as BroadcastAddr(%d)\n", deviceId, broadcastAddr);
    }
    seedOffset = GetInt(sec, _SeedOffset, 0xB1, 0, 255);
    fan1OnTemp = GetInt(sec, _Fan1OnTemp, 40, 0, 100 );
    fan2OnTemp = GetInt(sec, _Fan2OnTemp, 50, 0, 100 );
    overTemp = GetInt(sec, _OverTemp, 70, 0, 100);
    humidity = GetInt(sec, _Humidity, 70, 0, 100);
    defaultFont = GetInt(sec, _DefaultFont, 1, 1, 5);
    defaultColour = GetInt(sec, _DefaultColour, 1, 1, 9);
    lockedFrm = GetInt(sec, _LockedFrm, 0, 0, 255);
    lockedMsg = GetInt(sec, _LockedMsg, 0, 0, 255);
    passwordOffset = GetInt(sec, _PasswordOffset, 0x87A2, 0, 0xFFFF);
    displayTimeout = GetInt(sec, _DisplayTimeout, 21900, 0, 0xFFFF);
    sessionTimeout = GetInt(sec, _SessionTimeout, 180, 0, 0xFFFF);
    svcPort = GetInt(sec, _SvcPort, 38400, 1024, 0xFFFF);
    webPort = GetInt(sec, _WebPort, 38401, 1024, 0xFFFF);
    multiLedFaultThreshold = GetInt(sec, _MultiLedFaultThreshold, 16, 0, 0xFFFF);
    
    str = GetStr(sec, _ComportTMC);
    comportTMC = 0;
    if (str != NULL)
    {
        for (int i = 0; i < 7; i++)
        {
            if (strcmp(ComPort[i], str) == 0)
            {
                comportTMC = i;
                break;
            }
        }
    }
    baudrateTMC = GetInt(sec, "BaudrateTMC", 38400, 0, 115200);

    str = GetStr(sec, _ShakehandsPassword);
    if(str==NULL)
    {
        strcpy(shakehandsPassword, "Br1ghtw@y");
    }
    else
    {
        int len=strlen(str);
        if(len>10)len=10;
        memcpy(shakehandsPassword, str, len);
        shakehandsPassword[len]='\0';
    }

    strcpy(cbuf, ExtSw_Cfg);
    for(int m=0; m<3 ; m++)
    {
        cbuf[5]=m+'1'; // ExtSw_Cfg
        str = GetStr(sec, cbuf);
        cnt = Cnvt::GetIntArray(c, 4, ibuf, 0, 65535);
        if(cnt==4)
        {
            extSwCfg[m].dispTime = ibuf[0];
            extSwCfg[m].reserved = ibuf[1];
            extSwCfg[m].emergency = ibuf[2];
            extSwCfg[m].flashingOv = ibuf[3];
        }
    }

#define _TZ "TZ"
    str = GetStr(sec, _DawnDusk);
    if(str!=NULL)
    {
        cnt=Cnvt::GetIntArray(str, 16, ibuf, 0, 59);
        if(cnt==16)
        {
            for(cnt=0;cnt<16;cnt+=2)
            {
                if(ibuf[cnt]>23)
                {
                    MyThrow("DawnDusk Error: Hour>23");
                }
            }
            for(cnt=0;cnt<16;cnt++)
            {
                dawnDusk[cnt]=ibuf[cnt];
            }
        }
        else
        {
            MyThrow("DawnDusk Error: cnt!=16");
        }
    }
    str = GetStr(sec, _Luminance);
    cnt=GetIntArray(str, 16, ibuf, 1, 65535);
    if(cnt==16)
    {
        for(cnt=0;cnt<15;cnt++)
        {
            if(ibuf[cnt]>=ibuf[cnt+1])
            {
                MyThrow("Luminance Error: [%d]%d>[%d]%d",cnt,ibuf[cnt], cnt+1, ibuf[cnt+1]);
            }
        }
        for(cnt=0;cnt<16;cnt++)
        {
            luminance[cnt]=ibuf[cnt];
        }
    }
    else
    {
        MyThrow("Luminance Error: cnt!=16");
    }

    signN = 1;
    grpSign = new GrpSign[signN];
    // load groupsSign

    Close();
    Dump();
}

void UciUser::LoadFactoryDefault()
{
    SeedOffset(0xB1);
    PasswdOffset(0x87A2);
    SessionTimeout(120);
    DisplayTimeout(21900);
    SvcPort(38400);
    WebPort(38401);
    BaudrateTMC(38400);
}

void UciUser::Dump()
{
}

void UciUser::BroadcastId(uint8_t v)
{
    broadcastId=v;
    OptionSaveInt(_BroadcastId, v);
    isChanged = true;
}

void UciUser::DeviceId(uint8_t v)
{
    deviceId=v;
    OptionSaveInt(_DeviceId, v);
    isChanged = true;
}

void UciUser::SeedOffset(uint8_t v)
{
    isChanged = true;
}

void UciUser::PasswdOffset(uint16_t v)
{
    isChanged = true;
}

void UciUser::SessionTimeout(uint16_t v)
{
    isChanged = true;
}

void UciUser::DisplayTimeout(uint16_t v)
{
    isChanged = true;
}

void UciUser::SvcPort(uint16_t v)
{
    isChanged = true;
}

void UciUser::WebPort(uint16_t v)
{
    isChanged = true;
}

void UciUser::BaudrateTMC(int v)
{
    isChanged = true;
}

void UciUser::DawnDusk(uint8_t *p)
{
    memcpy(mDawnDusk, p, sizeof(mDawnDusk));
    char buf[128];
    int len = 0;
    char *pb = buf;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            unsigned char v = mDawnDusk[i * 2 + j];
            if (j == 0)
            {
                if (i == 0)
                {
                    len = sprintf(pb, "'%u:", v);
                }
                else
                {
                    len = sprintf(pb, "%u:", v);
                }
            }
            else
            {
                if (i == 7)
                {
                    len = sprintf(pb, "%02u'", v);
                }
                else
                {
                    len = sprintf(pb, "%02u,", v);
                }
            }
            pb += len;
        }
    }
    SetUciUserConfig(_DawnDusk, buf);
}

void UciUser::Luminance(uint16_t *p)
{
    memcpy(luminance, p, sizeof(luminance));
    char buf[128];
    int len = 0;
    char *pb = buf;
    for (int i = 0; i < 16; i++)
    {
        if (i == 0)
        {
            len = sprintf(pb, "'%u,", mLuminance[i]);
        }
        else if (i == 15)
        {
            len = sprintf(pb, "%u'", mLuminance[i]);
        }
        else
        {
            len = sprintf(pb, "%u,", mLuminance[i]);
        }
        pb += len;
    }
    SetUciUserConfig(_Luminance, buf);
}
