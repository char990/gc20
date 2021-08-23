#include <cstdio>
#include <cstring>
#include <uci/UciUser.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <module/Tz_AU.h>

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

UciUser::UciUser(UciProd &uciProd)
:uciProd(uciProd)
{
    PATH = "/etc/config";
    PACKAGE = "goblin_user";
    isChanged = false;
    groupCfg = new uint8_t[uciProd.Signs()];
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
        MyThrow("DeviceId(%d) should not be same as BroadcastId(%d)", deviceId, broadcastId);
    }
    seedOffset = GetInt(sec, _SeedOffset, 0, 255);
    fan1OnTemp = GetInt(sec, _Fan1OnTemp, 0, 100 );
    fan2OnTemp = GetInt(sec, _Fan2OnTemp, 0, 100 );
    overTemp = GetInt(sec, _OverTemp, 0, 100);
    humidity = GetInt(sec, _Humidity, 0, 100);
    defaultFont = GetInt(sec, _DefaultFont, 1, uciProd.MaxFont());
    defaultColour = GetInt(sec, _DefaultColour, 1, 9);
    for(int i=0;i<9;i++)
    {
        if(defaultColour)
        {
            MyThrow("DefaultColour(%d) is not valid", defaultColour);
        }
    }
    lockedFrm = GetInt(sec, _LockedFrm, 0, 255);
    lockedMsg = GetInt(sec, _LockedMsg, 0, 255);
    passwordOffset = GetInt(sec, _PasswordOffset, 0, 0xFFFF);
    displayTimeout = GetInt(sec, _DisplayTimeout, 0, 0xFFFF);
    sessionTimeout = GetInt(sec, _SessionTimeout, 0, 0xFFFF);
    svcPort = GetInt(sec, _SvcPort, 1024, 0xFFFF);
    webPort = GetInt(sec, _WebPort, 1024, 0xFFFF);
    if (deviceId == broadcastId)
    {
        MyThrow("SvcPort(%d) should not be same as WebPort(%d)", svcPort, webPort);
    }
    multiLedFaultThreshold = GetInt(sec, _MultiLedFaultThreshold, 0, 0xFFFF);
    baudrateTMC = GetInt(sec, _BaudrateTMC, 0, 115200);
    
    str = GetStr(sec, _ComportTMC);
    comportTMC = 7;
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
    if(comportTMC == 7)
    {
        MyThrow("ComportTMC error");
    }

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

    strcpy(cbuf, _ExtSw_Cfg);
    for(int m=0; m<3 ; m++)
    {
        cbuf[5]=m+'1'; // ExtSw_Cfg
        str = GetStr(sec, cbuf);
        cnt = Cnvt::GetIntArray(cbuf, 4, ibuf, 0, 65535);
        if(cnt==4)
        {
            extSwCfg[m].dispTime = ibuf[0];
            extSwCfg[m].reserved = ibuf[1];
            extSwCfg[m].emergency = ibuf[2];
            extSwCfg[m].flashingOv = ibuf[3];
        }
        else
        {
            MyThrow("%s error", cbuf);
        }
    }

	str = GetStr(sec, _TZ);
    if(str!=NULL)
    {
        for(int i=0;i<NUMBER_OF_TZ;i++)
        {
            if(strcasecmp(str,Tz_AU::tz_au[i].city)==0)
            {
                tz=i;
                break;
            }
        }
    }
    else
    {
        MyThrow("TZ error");
    }

    str = GetStr(sec, _DawnDusk);
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

    str = GetStr(sec, _Luminance);
    cnt=Cnvt::GetIntArray(str, 16, ibuf, 1, 65535);
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

    str = GetStr(sec, _GroupCfg);
    int signs=uciProd.Signs();
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
        MyThrow("GroupCfg Error: cnt!=%d",signs);
    }

    Close();
    Dump();
}

void UciUser::LoadFactoryDefault()
{
    char deffile[256];
    snprintf(deffile,255,"%s/%s.def", PATH,PACKAGE);
    char ucifile[256];
    snprintf(ucifile,255,"%s/%s", PATH,PACKAGE);
    Exec::CopyFile(deffile,ucifile);
}

void UciUser::Dump()
{
	printf ( "\n---------------\n" );
	printf ( "UCI User Configs:\n" );
	printf ( "---------------\n" );
    PrintOption_d(_DeviceId, DeviceId());
    PrintOption_d(_BroadcastId, BroadcastId());
    PrintOption_2x(_SeedOffset, SeedOffset());
    PrintOption_4x(_PasswordOffset, PasswordOffset());
    PrintOption_d(_SvcPort, SvcPort());
    PrintOption_d(_WebPort, WebPort());
    PrintOption_d(_BaudrateTMC, BaudrateTMC());
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

	printf ( "\t%s=%s\n", _TZ, TZ() );
	printf ( "\t%s=%s\n", _ComportTMC, ComPort[ComportTMC()] );
    
    char buf[256];

	for(int i=0;i<3;i++)
	{
		PrintExtSwCfg(i, buf);
        printf( "\tExtSw%dCfg=%s\n",i+1, buf);
	}

	PrintLuminance(buf);
	printf ( "\%s=%s\n", _Luminance, buf);

	PrintDawnDusk(buf);
	printf ( "\%s=%s\n", _DawnDusk, buf);

	PrintGroupCfg(buf);
	printf ( "\%s=%s\n", _GroupCfg, buf);

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
                len+=sprintf (buf+len, ":%u'", *p);
            }
            else
            {
                len+=sprintf (buf+len, ":%u,", *p);
            }
        }
        p++;
    }
}

void UciUser::PrintGroupCfg(char *buf)
{
    int signs=uciProd.Signs();
    uint8_t * gcfg=GroupCfg();
    int len=0;
    for(int i=0;i<signs;i++)
	{
		if(i==0)
        {
            len+=sprintf (buf+len, "'%u,", *(gcfg+i));
        }
        else if(i=signs-1)
        {
            len+=sprintf (buf+len, "%u'", *(gcfg+i));
        }
        else
        {
            len+=sprintf (buf+len, "%u,", *(gcfg+i));
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
            len+=sprintf (buf+len, "'%u,", *(lum+i));
        }
		if(i==15)
        {
            len+=sprintf (buf+len, "%u'", *(lum+i));
        }
        else
        {
            len+=sprintf (buf+len, "%u,", *(lum+i));
        }
    }
}

void UciUser::BroadcastId(uint8_t v)
{
    if(broadcastId!=v)
    {
        broadcastId=v;
        OptionSaveInt(_BroadcastId, v);
        isChanged = true;
    }
}

void UciUser::DeviceId(uint8_t v)
{
    if(deviceId!=v)
    {
        deviceId=v;
        OptionSaveInt(_DeviceId, v);
        isChanged = true;
    }
}

void UciUser::SeedOffset(uint8_t v)
{
}

void UciUser::PasswordOffset(uint16_t v)
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

void UciUser::Tz(uint8_t tz)
{
    isChanged = true;
}

const char * UciUser::TZ()
{
    return Tz_AU::tz_au[tz].city;
}

void UciUser::DawnDusk(uint8_t *p)
{
    memcpy(dawnDusk, p, sizeof(dawnDusk));
    char buf[1024];
    int len = 0;
    char *pb = buf;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            unsigned char v = dawnDusk[i * 2 + j];
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
    char buf[1024];
    int len = 0;
    char *pb = buf;
    for (int i = 0; i < 16; i++)
    {
        if (i == 0)
        {
            len = sprintf(pb, "'%u,", luminance[i]);
        }
        else if (i == 15)
        {
            len = sprintf(pb, "%u'", luminance[i]);
        }
        else
        {
            len = sprintf(pb, "%u,", luminance[i]);
        }
        pb += len;
    }
    SetUciUserConfig(_Luminance, buf);
}

void UciUser::SetUciUserConfig(const char *option, char * value)
{

}
