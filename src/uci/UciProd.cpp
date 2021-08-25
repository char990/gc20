#include <string>
#include <uci/UciProd.h>
#include <tsisp003/TsiSp003Const.h>


// index is coulur code
const char * COLOUR_NAME[10]={
    "DEFAULT",
    "RED",
    "YELLOW",
    "GREEN",
    "CYAN",
    "BLUE",
    "MAGENTA",
    "WHITE",
    "ORANGE",
    "AMBER"
};

UciProd::UciProd()
{
    PATH = "./config";
    PACKAGE = "gcprod";
    bFont.SetBit(0);
    bConspicuity.SetBit(0);
    bAnnulus.SetBit(0);
    bTxtFrmColour.SetBit(0);
    bGfxFrmColour.SetBit(0);
    bHrgFrmColour.SetBit(0);
    signs=nullptr;

}

UciProd::~UciProd()
{
    if(signs!=nullptr)
    {
        delete [] signs;
    }
}

void UciProd::LoadConfig()
{
    Open();
    sec = GetSection(SECTION_NAME);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;

    slaveRqstInterval=GetInt(sec, _SlaveRqstInterval, 10, 1000);
    slaveRqstStTo=GetInt(sec, _SlaveRqstStTo, 10, 1000);
    slaveRqstExtTo=GetInt(sec, _SlaveRqstExtTo, 10, 1000);
    if(slaveRqstStTo>slaveRqstInterval)
    {
        MyThrow("UciProd Error: SlaveRqstStTo(%d) > SlaveRqstInterval(%d)", slaveRqstStTo, slaveRqstInterval);
    }
    if(slaveRqstExtTo>slaveRqstInterval)
    {
        MyThrow("UciProd Error: SlaveRqstExtTo(%d) > SlaveRqstInterval(%d)", slaveRqstExtTo, slaveRqstInterval);
    }
    slaveSetStFrmDly=GetInt(sec, _SlaveSetStFrmDly, 10, 1000);
    slaveDispDly=GetInt(sec, _SlaveDispDly, 10, 1000);
    slaveCmdDly=GetInt(sec, _SlaveCmdDly, 10, 1000);
    if(slaveCmdDly>slaveSetStFrmDly)
    {
        MyThrow("UciProd Error: SlaveCmdDly(%d) > SlaveSetStFrmDly(%d)", slaveCmdDly, slaveSetStFrmDly);
    }
    if(slaveCmdDly>slaveDispDly)
    {
        MyThrow("UciProd Error: SlaveCmdDly(%d) > SlaveDispDly(%d)", slaveCmdDly, slaveDispDly);
    }

    lightSensorMidday=GetInt(sec, _LightSensorMidday, 1, 65535);
    lightSensorMidnight=GetInt(sec, _LightSensorMidnight, 1, 65535);
    lightSensor18Hours=GetInt(sec, _LightSensor18Hours, 1, 65535);
    driverFaultDebounce=GetInt(sec, _DriverFaultDebounce, 1, 65535);
    overTempDebounce=GetInt(sec, _OverTempDebounce, 1, 65535);
    selftestDebounce=GetInt(sec, _SelftestDebounce, 1, 65535);
    offlineDebounce=GetInt(sec, _OfflineDebounce, 1, 65535);
    lightSensorFaultDebounce=GetInt(sec, _LightSensorFaultDebounce, 1, 65535);
    lanternFaultDebounce=GetInt(sec, _LanternFaultDebounce, 1, 65535);
    slaveVoltageLow=GetInt(sec, _SlaveVoltageLow, 1, 65535);
    slaveVoltageHigh=GetInt(sec, _SlaveVoltageHigh, 1, 65535);
    if(slaveVoltageLow>slaveVoltageHigh)
    {
        MyThrow("UciProd Error: SlaveVoltageLow(%d) > SlaveVoltageHigh(%d)", slaveVoltageLow, slaveVoltageHigh);
    }
    lightSensorScale=GetInt(sec, _LightSensorScale, 1, 65535);

    slavePowerUpDelay=GetInt(sec, _SlavePowerUpDelay, 1, 255);
    colourBits=GetInt(sec, _ColourBits, 1, 24);
    if(colourBits!=1 && colourBits!=4 && colourBits!=24)
    {
        MyThrow("UciProd Error: ColourBits(%d) Only 1/4/24 allowed", colourBits);
    }
    isResetLogAllowed=GetInt(sec, _IsResetLogAllowed, 0, 1);
    isUpgradeAllowed=GetInt(sec, _IsUpgradeAllowed, 0, 1);
    numberOfSigns=GetInt(sec, _NumberOfSigns, 1, 16);
    signs = new struct SignConnection[numberOfSigns];
    strcpy(cbuf, "Sing_");
    for(int i=1;i<=numberOfSigns;i++)
    {
        cbuf[4]=i+'0';
        str = GetStr(sec, cbuf);
        uint32_t x1,x2,x3,x4,x5;
        if(sscanf(str, "%u.%u.%u.%u:%u",&x1,&x2,&x3,&x4,&x5)==5)
        {
            if(x1!=0 && x1<256 && x2<256 && x3<256 && x4!=0 && x4<255 && x5>1024 && x5<65536)
            {
                signs[i-1].com_ip = ((x1*0x100+x2)*0x100+x3)*0x100+x4;
                signs[i-1].bps_port = x5;
                continue;
            }
        }
        else
        {
            for(cnt=0;cnt<COMPORT_SIZE;cnt++)
            {
                if(memcmp(str, COMPORTS[cnt].name, 4)==0)
                    break;
            }
            if(cnt<COMPORT_SIZE)
            {
                char * bps = strchr(str,':');
                if(bps!=NULL)
                {
                    bps++;
                    if(sscanf(bps, "%u", &x5)==1)
                    {
                        for(cnt=0;cnt<EXTENDEDBPS_SIZE;cnt++)
                        {
                            if(ALLOWEDBPS[cnt]==x5)
                                break;
                        }
                        if(cnt<EXTENDEDBPS_SIZE)
                        {
                            signs[i-1].com_ip = cnt;
                            signs[i-1].bps_port = x5;
                            continue;
                        }
                    }
                }
            }
        }
        MyThrow("UciProd Error: %s '%s'", cbuf, str);
    }



    str = GetStr(sec, _ComPort);
    comPort = COMPORT_SIZE;
    if (str != NULL)
    {
        for (int i = 0; i < COMPORT_SIZE; i++)
        {
            if (strcmp(COMPORTS[i], str) == 0)
            {
                comPort = i;
                break;
            }
        }
    }
    if(comPort == COMPORT_SIZE)
    {
        MyThrow("UciUser::Unknown ComPort:%s", str);
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
        for(int i=0;i<NUMBER_OF_TZ;i++)
        {
            if(strcasecmp(str,Tz_AU::tz_au[i].city)==0)
            {
                tz=i;
                break;
            }
        }
    }
    if(i==NUMBER_OF_TZ)
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

    str = GetStr(sec, _GroupCfg);
    int signs=uciProd.NumberOfSigns();
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

void UciProd::Dump()
{


}

uint8_t UciProd::TsiSp003Ver()
{
    return 0x50;
}


char * UciProd::MfcCode()
{
    return  const_cast<char *>("GC20123456");
}

int UciProd::ColourBits()
{
    return 1;
    return 4;
    return 24;
}

int UciProd::MaxTextFrmLen()
{
    return 255;
}

int UciProd::MinGfxFrmLen() 
{
    return (Pixels()+7)/8;
}

int UciProd::MaxGfxFrmLen() 
{
    switch(ColourBits())
    {
        case 1:
            return (Pixels()+7)/8;
        case 4:
            return (Pixels()+2)/2;
        default:
            return 0;
    }
}

int UciProd::MinHrgFrmLen() 
{
    return (Pixels()+7)/8;
}

int UciProd::MaxHrgFrmLen() 
{
    switch(ColourBits())
    {
        case 1:
            return (Pixels()+7)/8;
        case 4:
            return (Pixels()+2)/2;
        default:
            return (Pixels()*3);
    }
}

int UciProd::CharRows() 
{
    return 3;
}

int UciProd::CharColumns() 
{
    return 18;
}

int UciProd::PixelRows() 
{
    return 64;
}

int UciProd::PixelColumns() 
{
    return 288;
}

int UciProd::Pixels() 
{
    return PixelRows()*PixelColumns();
}

int UciProd::NumberOfSigns() 
{
    return 1;
}

int UciProd::MaxFont()
{
    return 2;
}
