#include <cstdio>
#include <cstring>
#include <uci/UciUser.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <module/Tz_AU.h>
#include <module/SerialPort.h>

using namespace Utils;

UciUser::UciUser(UciProd &uciProd)
:uciProd(uciProd)
{
    PATH = "./config";
    PACKAGE = "gcuser";
    DEFAULT_FILE = "./config/gcuser.def";
    groupCfg = new uint8_t[uciProd.NumberOfSigns()];
}

UciUser::~UciUser()
{
    delete[] groupCfg;
}

void UciUser::LoadConfig()
{
    Open();
    sec = GetSection(SECTION_NAME);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;

    deviceId = GetInt(sec, _DeviceId, 0, 255);
    broadcastId = GetInt(sec, _BroadcastId, 0, 255);
    if (deviceId == broadcastId)
    {
        MyThrow("UciUser::DeviceId(%d) should not be same as BroadcastId(%d)", deviceId, broadcastId);
    }
    seedOffset = GetInt(sec, _SeedOffset, 0, 255);
    fan1OnTemp = GetInt(sec, _Fan1OnTemp, 0, 100 );
    fan2OnTemp = GetInt(sec, _Fan2OnTemp, 0, 100 );
    overTemp = GetInt(sec, _OverTemp, 0, 100);
    humidity = GetInt(sec, _Humidity, 0, 100);
    defaultFont = GetInt(sec, _DefaultFont, 1, MAX_FONT);
    if(!uciProd.IsFont(defaultFont))
    {
        MyThrow("UciUser::DefaultFont(%d) is not valid", defaultFont);
    }
    defaultColour = GetInt(sec, _DefaultColour, 1, MAX_MONOCOLOUR);
    if(!uciProd.IsTxtFrmColour(defaultColour))
    {
        MyThrow("UciUser::DefaultColour(%d) is not valid in TextFrameColour", defaultColour);
    }
    if(!uciProd.IsGfxFrmColour(defaultColour))
    {
        MyThrow("UciUser::DefaultColour(%d) is not valid in GfxFrameColour", defaultColour);
    }
    if(!uciProd.IsHrgFrmColour(defaultColour))
    {
        MyThrow("UciUser::DefaultColour(%d) is not valid in HrgFrameColour", defaultColour);
    }

    lockedFrm = GetInt(sec, _LockedFrm, 0, 255);
    lockedMsg = GetInt(sec, _LockedMsg, 0, 255);
    passwordOffset = GetInt(sec, _PasswordOffset, 0, 0xFFFF);
    displayTimeout = GetInt(sec, _DisplayTimeout, 0, 0xFFFF);
    sessionTimeout = GetInt(sec, _SessionTimeout, 0, 0xFFFF);
    svcPort = GetInt(sec, _SvcPort, 1024, 0xFFFF);
    webPort = GetInt(sec, _WebPort, 1024, 0xFFFF);
    if (svcPort == webPort)
    {
        MyThrow("UciUser::SvcPort(%d) should not be same as WebPort(%d)", svcPort, webPort);
    }
    multiLedFaultThreshold = GetInt(sec, _MultiLedFaultThreshold, 0, 0xFFFF);

    baudrate = GetInt(sec, _Baudrate, ALLOWEDBPS[0], ALLOWEDBPS[STANDARDBPS_SIZE-1]);
    {
        for(cnt=0;cnt<STANDARDBPS_SIZE;cnt++)
        {
            if(baudrate==ALLOWEDBPS[cnt])break;
        }
        if(cnt==STANDARDBPS_SIZE)
        {
            MyThrow("UciUser::Unknown Baudrate");
        }
    }

    str = GetStr(sec, _ShakehandsPassword);
    if(str==NULL)
    {
        strcpy(shakehandsPassword, "brightway");
    }
    else
    {
        int len=strlen(str);
        if(len>10)len=10;
        memcpy(shakehandsPassword, str, len);
        shakehandsPassword[len]='\0';
    }

    strcpy(cbuf, _ExtSw_Cfg);
    for(int m=0; m<3 ; m++)
    {
        cbuf[5]=m+'1'; // ExtSw_Cfg
        str = GetStr(sec, cbuf);
        cnt = Cnvt::GetIntArray(str, 4, ibuf, 0, 65535);
        if(cnt==4)
        {
            extSwCfg[m].dispTime = ibuf[0];
            extSwCfg[m].reserved = ibuf[1];
            extSwCfg[m].emergency = ibuf[2];
            extSwCfg[m].flashingOv = ibuf[3];
        }
        else
        {
            MyThrow("UciUser::%s error: %s", cbuf, str);
        }
    }

	str = GetStr(sec, _TZ);
    tz=NUMBER_OF_TZ;
    if(str!=NULL)
    {
        for(int cnt=0;cnt<NUMBER_OF_TZ;cnt++)
        {
            if(strcasecmp(str,Tz_AU::tz_au[cnt].city)==0)
            {
                tz=cnt;
                break;
            }
        }
    }
    if(cnt==NUMBER_OF_TZ)
    {
        MyThrow("UciUser::TZ error:%s",str);
    }

    str = GetStr(sec, _DawnDusk);
    cnt=Cnvt::GetIntArray(str, 16, ibuf, 0, 59);
    if(cnt==16)
    {
        for(cnt=0;cnt<16;cnt+=2)
        {
            if(ibuf[cnt]>23)
            {
                MyThrow("UciUser::DawnDusk Error: Hour>23");
            }
        }
        for(cnt=0;cnt<16;cnt++)
        {
            dawnDusk[cnt]=ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciUser::DawnDusk Error: cnt!=16");
    }

    str = GetStr(sec, _Luminance);
    cnt=Cnvt::GetIntArray(str, 16, ibuf, 1, 65535);
    if(cnt==16)
    {
        for(cnt=0;cnt<15;cnt++)
        {
            if(ibuf[cnt]>=ibuf[cnt+1])
            {
                MyThrow("UciUser::Luminance Error: [%d]%d>[%d]%d",cnt,ibuf[cnt], cnt+1, ibuf[cnt+1]);
            }
        }
        for(cnt=0;cnt<16;cnt++)
        {
            luminance[cnt]=ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciUser::Luminance Error: cnt!=16");
    }

    int signs=uciProd.NumberOfSigns();

    str = GetStr(sec, _ComPort);
    comPort = COMPORT_SIZE;
    if (str != NULL)
    {
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            if (strcmp(COMPORTS[i].name, str) == 0)
            {
                comPort = i;
                break;
            }
        }
    }
    for(int i=0;i<signs;i++)
    {
        if(comPort==uciProd.Sign(i)->com_ip)
        {
            MyThrow("UciUser::%s: %s assigned to Sign%d", _ComPort, COMPORTS[i].name, i+1);
        }
    }
    if(comPort == COMPORT_SIZE)
    {
        MyThrow("UciUser::Unknown ComPort:%s", str);
    }

    str = GetStr(sec, _GroupCfg);
    cnt=Cnvt::GetIntArray(str, signs, ibuf, 1, signs);
    if(cnt==signs)
    {
        for(cnt=0;cnt<signs;cnt++)
        {
            groupCfg[cnt]=ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciUser::GroupCfg Error: cnt!=%d",signs);
    }

    Close();
    Dump();
}

void UciUser::LoadFactoryDefault()
{
    char ucifile[256];
    snprintf(ucifile,255,"%s/%s", PATH,PACKAGE);
    Exec::CopyFile(DEFAULT_FILE,ucifile);
	UserOpen();
    OptionSaveInt(_DeviceId, DeviceId());
    OptionSaveInt(_BroadcastId, BroadcastId());
	UserClose();
}

void UciUser::UserOpen()
{
    OpenSectionForSave(SECTION_NAME);
}

void UciUser::UserClose()
{
    CloseSectionForSave();
}

void UciUser::Dump()
{
	printf ( "\n---------------\n" );
	printf ( "%s/%s.%s\n", PATH, PACKAGE, SECTION_NAME);
	printf ( "---------------\n" );
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
    PrintOption_d(_LastFrmTime, LastFrmTime());

	printf ( "\t%s '%s'\n", _TZ, TZ() );
	printf ( "\t%s '%s'\n", _ComPort, COMPORTS[ComPort()] );
    
    char buf[256];

    int len=sprintf(buf,"%s ", _ExtSw_Cfg);
    for(int i=0;i<3;i++)
	{
		buf[5]=i+'1';
        PrintExtSwCfg(i, buf+len);
        printf("\t%s\n",buf);
	}

	PrintLuminance(buf);
	printf ( "\t%s %s\n", _Luminance, buf);

	PrintDawnDusk(buf);
	printf ( "\t%s %s\n", _DawnDusk, buf);

	PrintGroupCfg(buf);
	printf ( "\t%s %s\n", _GroupCfg, buf);

	printf ( "\n---------------\n" );
}

void UciUser::PrintExtSwCfg(int i, char *buf)
{
    ExtSwCfg * exswcfg=ExtSwCfgX(i);
    sprintf (buf, "'%d,%d,%d,%d'",
		exswcfg->dispTime, exswcfg->reserved, exswcfg->emergency, exswcfg->flashingOv);

}

void UciUser::PrintDawnDusk(char *buf)
{
    uint8_t *p = DawnDusk();
    int len=0;
    for(int i=0;i<16;i++)
    {
        if((i&1)==0)
        {
            if(i==0)
            {
                len+=sprintf (buf+len, "'%u:", *p);
            }
            else
            {
                len+=sprintf (buf+len, "%u:", *p,*(p+1));
            }
        }
        else
        {
            if(i==15)
            {
                len+=sprintf (buf+len, "%02u'", *p);
            }
            else
            {
                len+=sprintf (buf+len, "%02u,", *p);
            }
        }
        p++;
    }
}

void UciUser::PrintGroupCfg(char *buf)
{
    int signs=uciProd.NumberOfSigns();
    uint8_t * gcfg=GroupCfg();
    int len=0;
    for(int i=0;i<signs;i++)
	{
		if(i==0)
        {
            len+=sprintf (buf+len, "'%u", *(gcfg+i));
        }
        else
        {
            len+=sprintf (buf+len, ",%u", *(gcfg+i));
        }
        if(i==signs-1)
        {
            len+=sprintf (buf+len, "'");
        }
    }
}

void UciUser::PrintLuminance(char *buf)
{
    uint16_t * lum=Luminance();
	int len=0;
    for(int i=0;i<16;i++)
	{
		if(i==0)
        {
            len+=sprintf (buf+len, "'%u", *(lum+i));
        }
        else
        {
            len+=sprintf (buf+len, ",%u", *(lum+i));
        }
		if(i==15)
        {
            len+=sprintf (buf+len, "'");
        }
    }
}


    /// --------setter--------

void UciUser::BroadcastId(uint8_t v)
{
    if(broadcastId!=v)
    {
        broadcastId=v;
        OptionSaveInt(_BroadcastId, v);
    }
}

void UciUser::DeviceId(uint8_t v)
{
    if(deviceId!=v)
    {
        deviceId=v;
        OptionSaveInt(_DeviceId, v);
    }
}

void UciUser::SeedOffset(uint8_t v)
{
    if(seedOffset!=v)
    {
        seedOffset=v;
        OptionSaveInt(_SeedOffset, v);
    }
}

void UciUser::Fan1OnTemp(uint8_t v)
{
    if(fan1OnTemp!=v)
    {
        fan1OnTemp=v;
        OptionSaveInt(_Fan1OnTemp, v);
    }
}

void UciUser::Fan2OnTemp(uint8_t v)
{
    if(fan2OnTemp!=v)
    {
        fan2OnTemp=v;
        OptionSaveInt(_Fan2OnTemp, v);
    }
}

void UciUser::OverTemp(uint8_t v)
{
    if(overTemp!=v)
    {
        overTemp=v;
        OptionSaveInt(_OverTemp, v);
    }
}

void UciUser::Humidity(uint8_t v)
{
    if(humidity!=v)
    {
        humidity=v;
        OptionSaveInt(_Humidity, v);
    }
}

void UciUser::DefaultFont(uint8_t v)
{
    if(defaultFont!=v)
    {
        defaultFont=v;
        OptionSaveInt(_DefaultFont, v);
    }
}

void UciUser::DefaultColour(uint8_t v)
{
    if(defaultColour!=v)
    {
        defaultColour=v;
        OptionSaveInt(_DefaultColour, v);
    }
}

void UciUser::LockedFrm(uint8_t v)
{
    if(lockedFrm!=v)
    {
        lockedFrm=v;
        OptionSaveInt(_LockedFrm, v);
    }
}

void UciUser::LockedMsg(uint8_t v)
{
    if(lockedMsg!=v)
    {
        lockedMsg=v;
        OptionSaveInt(_LockedMsg, v);
    }
}

void UciUser::LastFrmTime(uint8_t v)
{
    if(lastFrmTime!=v)
    {
        lastFrmTime=v;
        OptionSaveInt(_LastFrmTime, v);
    }
}

void UciUser::ComPort(uint8_t v)
{
    if(comPort!=v)
    {
        comPort=v;
        OptionSaveChars(_ComPort, COMPORTS[comPort].name);
    }
}

void UciUser::MultiLedFaultThreshold(uint16_t v)
{
    if(multiLedFaultThreshold!=v)
    {
        multiLedFaultThreshold=v;
        OptionSaveInt(_MultiLedFaultThreshold, v);
    }
}


void UciUser::PasswordOffset(uint16_t v)
{
    if(passwordOffset!=v)
    {
        passwordOffset=v;
        OptionSaveInt(_PasswordOffset, v);
    }
}

void UciUser::SessionTimeout(uint16_t v)
{
    if(sessionTimeout!=v)
    {
        sessionTimeout=v;
        OptionSaveInt(_SessionTimeout, v);
    }
}

void UciUser::DisplayTimeout(uint16_t v)
{
    if(displayTimeout!=v)
    {
        displayTimeout=v;
        OptionSaveInt(_DisplayTimeout, v);
    }
}

void UciUser::SvcPort(uint16_t v)
{
    if(svcPort!=v)
    {
        svcPort=v;
        OptionSaveInt(_SvcPort, v);
    }
}

void UciUser::WebPort(uint16_t v)
{
    if(webPort!=v)
    {
        webPort=v;
        OptionSaveInt(_WebPort, v);
    }
}

void UciUser::Baudrate(int v)
{
    if(baudrate!=v)
    {
        baudrate=v;
        OptionSaveInt(_Baudrate, v);
    }
}

void UciUser::Tz(uint8_t v)
{
    if(tz!=v)
    {
        tz=v;
        OptionSaveChars(_TZ, Tz_AU::tz_au[tz].city);
    }
}

const char * UciUser::TZ()
{
    return Tz_AU::tz_au[tz].city;
}

void UciUser::DawnDusk(uint8_t *p)
{
    if(memcmp(p, dawnDusk, sizeof(dawnDusk))!=0)
    {
        memcpy(dawnDusk, p, sizeof(dawnDusk));
        char buf[1024];
        PrintLuminance(buf);
        OptionSave(_Luminance, buf);
    }
}

void UciUser::Luminance(uint16_t *p)
{
    if(memcmp(luminance, p, sizeof(luminance))!=0)
    {
        memcpy(luminance, p, sizeof(luminance));
        char buf[1024];
        PrintLuminance(buf);
        OptionSave(_Luminance, buf);
    }
}

void UciUser::GroupCfg(uint8_t *p)
{
    if(memcmp(groupCfg, p, uciProd.NumberOfSigns())!=0)
    {
        memcpy(groupCfg, p, uciProd.NumberOfSigns());
        char buf[1024];
        PrintGroupCfg(buf);
        OptionSave(_GroupCfg, buf);
    }
}

void UciUser::ExtSwCfgX(int i, ExtSwCfg *cfg)
{
    if(!extSwCfg[i].Equal(cfg))
    {
        memcpy(&extSwCfg[i], cfg, sizeof(ExtSwCfg));
        char op[32];
        strcpy(op,_ExtSw_Cfg);
        op[5]=i+'1';
        char buf[1024];
        PrintExtSwCfg(i,buf);
        OptionSave(op, buf);
    }
}
