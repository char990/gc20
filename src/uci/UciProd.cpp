#include <string>
#include <cstring>
#include <sys/stat.h>
#include <uci/UciProd.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/SerialPort.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <sign/SignFactory.h>

extern const char *FirmwareMajorVer;
extern const char *FirmwareMinorVer;

using namespace Utils;

std::string SignConnection::ToString()
{
    char buf[32];
    if (com_ip < COMPORT_SIZE)
    {
        sprintf(buf, "%s:%d", COMPORTS[com_ip].name, bps_port);
    }
    else
    {
        sprintf(buf, "%d.%d.%d.%d:%d",
                (com_ip >> 24) & 0xFF, (com_ip >> 16) & 0xFF, (com_ip >> 8) & 0xFF, com_ip & 0xFF, bps_port);
    }
    std::string s{buf};
    return s;
}

// index is coulur code
const char *COLOUR_NAME[10] = {
    "DEFAULT",
    "RED",
    "YELLOW",
    "GREEN",
    "CYAN",
    "BLUE",
    "MAGENTA",
    "WHITE",
    "ORANGE",
    "AMBER"};

UciProd::UciProd()
{
    PATH = "./config";
    PACKAGE = "gcprod";
    signCn = nullptr;
    for(int i=0;i<MAX_FONT+1;i++)
    {
        fonts[i]=nullptr;
    }
}

UciProd::~UciProd()
{
    if (signCn != nullptr)
    {
        delete[] signCn;
    }
    for(int i=0;i<MAX_FONT+1;i++)
    {
        Font * v=fonts[i];
        if(v!=nullptr)
        {
            delete fonts[i];
            for(int j=i;j<MAX_FONT+1;j++)
            {
                if(v==fonts[j])
                {
                    fonts[j]=nullptr;
                }
            }
        }
    }
}

void UciProd::LoadConfig()
{
    Open();
    struct uci_section *uciSec = GetSection(SECTION_NAME);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;

    tsiSp003Ver = SelectStr(uciSec, _TsiSp003Ver, TSISP003VER, TSISP003VER_SIZE);
    signType = SelectStr(uciSec, _SignType, SIGNTYPE, SIGNTYPE_SIZE);

    pixelRowsPerTile = GetInt(uciSec, _PixelRowsPerTile, 4, 255);
    pixelColumnsPerTile = GetInt(uciSec, _PixelColumnsPerTile, 8, 255);
    tileRowsPerSlave = GetInt(uciSec, _TileRowsPerSlave, 1, 32);
    tileColumnsPerSlave = GetInt(uciSec, _TileColumnsPerSlave, 1, 32);
    slaveRowsPerSign = GetInt(uciSec, _SlaveRowsPerSign, 1, 16);
    slaveColumnsPerSign = GetInt(uciSec, _SlaveColumnsPerSign, 1, 16);

    str = GetStr(uciSec, _MfcCode);
    if (strlen(str) == 6)
    {
        sprintf(mfcCode, "%s%s%s", str, FirmwareMajorVer, FirmwareMinorVer);
    }
    else
    {
        MyThrow("UciProd Error: MfcCode length should be 6");
    }

    slaveRqstInterval = GetInt(uciSec, _SlaveRqstInterval, 10, 1000);
    slaveRqstStTo = GetInt(uciSec, _SlaveRqstStTo, 10, 1000);
    slaveRqstExtTo = GetInt(uciSec, _SlaveRqstExtTo, 10, 1000);
    if (slaveRqstStTo > slaveRqstInterval)
    {
        MyThrow("UciProd Error: SlaveRqstStTo(%d) > SlaveRqstInterval(%d)", slaveRqstStTo, slaveRqstInterval);
    }
    if (slaveRqstExtTo > slaveRqstInterval)
    {
        MyThrow("UciProd Error: SlaveRqstExtTo(%d) > SlaveRqstInterval(%d)", slaveRqstExtTo, slaveRqstInterval);
    }
    slaveSetStFrmDly = GetInt(uciSec, _SlaveSetStFrmDly, 10, 1000);
    slaveDispDly = GetInt(uciSec, _SlaveDispDly, 10, 1000);
    slaveCmdDly = GetInt(uciSec, _SlaveCmdDly, 10, 1000);
    if (slaveCmdDly > slaveSetStFrmDly)
    {
        MyThrow("UciProd Error: SlaveCmdDly(%d) > SlaveSetStFrmDly(%d)", slaveCmdDly, slaveSetStFrmDly);
    }
    if (slaveCmdDly > slaveDispDly)
    {
        MyThrow("UciProd Error: SlaveCmdDly(%d) > SlaveDispDly(%d)", slaveCmdDly, slaveDispDly);
    }

    lightSensorMidday = GetInt(uciSec, _LightSensorMidday, 1, 65535);
    lightSensorMidnight = GetInt(uciSec, _LightSensorMidnight, 1, 65535);
    lightSensor18Hours = GetInt(uciSec, _LightSensor18Hours, 1, 65535);
    driverFaultDebounce = GetInt(uciSec, _DriverFaultDebounce, 1, 65535);
    overTempDebounce = GetInt(uciSec, _OverTempDebounce, 1, 65535);
    selftestDebounce = GetInt(uciSec, _SelftestDebounce, 1, 65535);
    offlineDebounce = GetInt(uciSec, _OfflineDebounce, 1, 65535);
    lightSensorFaultDebounce = GetInt(uciSec, _LightSensorFaultDebounce, 1, 65535);
    lanternFaultDebounce = GetInt(uciSec, _LanternFaultDebounce, 1, 65535);
    slaveVoltageLow = GetInt(uciSec, _SlaveVoltageLow, 1, 65535);
    slaveVoltageHigh = GetInt(uciSec, _SlaveVoltageHigh, 1, 65535);
    if (slaveVoltageLow > slaveVoltageHigh)
    {
        MyThrow("UciProd Error: SlaveVoltageLow(%d) > SlaveVoltageHigh(%d)", slaveVoltageLow, slaveVoltageHigh);
    }
    lightSensorScale = GetInt(uciSec, _LightSensorScale, 1, 65535);

    slavePowerUpDelay = GetInt(uciSec, _SlavePowerUpDelay, 1, 255);
    colourBits = GetInt(uciSec, _ColourBits, 1, 24);
    if (colourBits != 1 && colourBits != 4 && colourBits != 24)
    {
        MyThrow("UciProd Error: ColourBits(%d) Only 1/4/24 allowed", colourBits);
    }
    isResetLogAllowed = GetInt(uciSec, _IsResetLogAllowed, 0, 1);
    isUpgradeAllowed = GetInt(uciSec, _IsUpgradeAllowed, 0, 1);
    numberOfSigns = GetInt(uciSec, _NumberOfSigns, 1, 16);

    signCn = new struct SignConnection[numberOfSigns];
    for (int i = 1; i <= numberOfSigns; i++)
    {
        sprintf(cbuf, "%s%d", _Sign, i);
        str = GetStr(uciSec, cbuf);
        uint32_t x1, x2, x3, x4, x5;
        if (sscanf(str, "%u.%u.%u.%u:%u", &x1, &x2, &x3, &x4, &x5) == 5)
        {
            if (x1 != 0 && x1 < 256 && x2 < 256 && x3 < 256 && x4 != 0 && x4 < 255 && x5 > 1024 && x5 < 65536)
            {
                signCn[i - 1].com_ip = ((x1 * 0x100 + x2) * 0x100 + x3) * 0x100 + x4;
                signCn[i - 1].bps_port = x5;
                continue;
            }
        }
        else
        {
            for (cnt = 0; cnt < COMPORT_SIZE; cnt++)
            {
                if (memcmp(str, COMPORTS[cnt].name, 4) == 0)
                    break;
            }
            if (cnt < COMPORT_SIZE)
            {
                signCn[i - 1].com_ip = cnt;
                const char *bps = strchr(str, ':');
                if (bps != NULL)
                {
                    bps++;
                    if (sscanf(bps, "%u", &x5) == 1)
                    {
                        for (cnt = 0; cnt < EXTENDEDBPS_SIZE; cnt++)
                        {
                            if (ALLOWEDBPS[cnt] == x5)
                                break;
                        }
                        if (cnt < EXTENDEDBPS_SIZE)
                        {
                            signCn[i - 1].bps_port = x5;
                            continue;
                        }
                    }
                }
            }
        }
        MyThrow("UciProd Error: %s '%s'", cbuf, str);
    }

    ReadBitOption(uciSec, _Font, bFont);
    if(!bFont.GetBit(0))
    {
        MyThrow("UciProd Error: Font : Default(0) is not enabled");
    }
    ReadBitOption(uciSec, _Conspicuity, bConspicuity);
    ReadBitOption(uciSec, _Annulus, bAnnulus);
    ReadBitOption(uciSec, _TxtFrmColour, bTxtFrmColour);
    ReadBitOption(uciSec, _GfxFrmColour, bGfxFrmColour);
    ReadBitOption(uciSec, _HrgFrmColour, bHrgFrmColour);

    // Font0 'P5X7'
    // Font1 'F5X7'
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        if (bFont.GetBit(i))
        {
            sprintf(cbuf, "%s%d", _Font, i);
            str = GetStr(uciSec, cbuf);
            if(str==NULL)
            {
                MyThrow("UciProd Error: There is no option %s", cbuf);
            }
            if(strlen(str) > 15)
            {
                MyThrow("UciProd Error: %s '%s'(file name length too long >15)", cbuf, str);
            }
            if(i>0)
            {
                int j;
                for(j=0;j<i;i++)
                {
                    if(strcmp(str, fonts[j]->FontName())==0)
                    {
                        break;
                    }
                }
                if(j<i)
                {// found a loaded font
                    fonts[i] = fonts[j];
                }
                else
                {// new font to load
                    fonts[i] = new Font(str);
                }
            }
        }
    }

    mappedColoursTable[0]=0;
    for(int i=1;i<10,i++)
    {
        str = GetStr(uciSec, COLOUR_NAME[i]);
        for(cnt=1;cnt<10;cnt++)
        {
            if(strcmp(str, COLOUR_NAME[cnt])==0)
            {
                mappedColoursTable[i]=cnt;
                break;
            }
        }
        if(cnt==10)
        {
            MyThrow("UciProd Error: colour map %s undefined", COLOUR_NAME[i]);
        }
    }

    Close();
    Dump();

    
    pixelRows = (uint16_t)pixelRowsPerTile * tileRowsPerSlave * slaveRowsPerSign;
    pixelColumns = (uint16_t)pixelColumnsPerTile * tileColumnsPerSlave * slaveColumnsPerSign;
    pixels = (uint32_t)pixelRows * pixelColumns;

    extStsRplSignType = mfcCode[5]-'0';
    if(extStsRplSignType==0)
    {
        configRplSignType=0;
    }
    else if(extStsRplSignType==1 || extStsRplSignType==2)
    {
        switch(colourBits)
        {
            case 1:
                configRplSignType = 1;
            case 4:
                configRplSignType = 2;
            case 24:
                configRplSignType = 3;
            default:
                MyThrow("Unknown ColourBits in UciProd");
        }
    }
    else
    {
        MyThrow("Unknown extStSignType:%d",extStsRplSignType);
    }

    minGfxFrmLen = (pixels + 7) / 8;
    switch (colourBits)
    {
    case 1:
        maxGfxFrmLen = (pixels + 7) / 8;
    case 4:
        maxGfxFrmLen = (pixels + 1) / 2;
    default:
        maxGfxFrmLen = pixels*3;
    }
}

void UciProd::Dump()
{
    printf("\n---------------\n");
    printf("%s/%s.%s\n", PATH, PACKAGE, SECTION_NAME);
    printf("---------------\n");

    printf("\t%s '%s'\n", _TsiSp003Ver, TSISP003VER[TsiSp003Ver()]);
    printf("\t%s '%s'\n", _SignType, SIGNTYPE[SignType()]);
    printf("\t%s '%s'\n", _MfcCode, MfcCode());

    PrintOption_d(_NumberOfSigns, NumberOfSigns());
    for (int i = 0; i < NumberOfSigns(); i++)
    {
        printf("\t%s%d '%s'\n", _Sign, i, Sign(i)->ToString().c_str());
    }
    PrintOption_d(_PixelRowsPerTile, PixelRowsPerTile());
    PrintOption_d(_PixelColumnsPerTile, PixelColumnsPerTile());
    PrintOption_d(_TileRowsPerSlave, TileRowsPerSlave());
    PrintOption_d(_TileColumnsPerSlave, TileColumnsPerSlave());
    PrintOption_d(_SlaveRowsPerSign, SlaveRowsPerSign());
    PrintOption_d(_SlaveColumnsPerSign, SlaveColumnsPerSign());

    PrintOption_d(_SlaveRqstInterval, SlaveRqstInterval());
    PrintOption_d(_SlaveRqstStTo, SlaveRqstStTo());
    PrintOption_d(_SlaveRqstExtTo, SlaveRqstExtTo());
    PrintOption_d(_SlaveSetStFrmDly, SlaveSetStFrmDly());
    PrintOption_d(_SlaveDispDly, SlaveDispDly());
    PrintOption_d(_SlaveCmdDly, SlaveCmdDly());
    PrintOption_d(_LightSensorMidday, LightSensorMidday());
    PrintOption_d(_LightSensorMidnight, LightSensorMidnight());
    PrintOption_d(_LightSensor18Hours, LightSensor18Hours());
    PrintOption_d(_DriverFaultDebounce, DriverFaultDebounce());
    PrintOption_d(_LedFaultDebounce, LedFaultDebounce());
    PrintOption_d(_OverTempDebounce, OverTempDebounce());
    PrintOption_d(_SelftestDebounce, SelftestDebounce());
    PrintOption_d(_OfflineDebounce, OfflineDebounce());
    PrintOption_d(_LightSensorFaultDebounce, LightSensorFaultDebounce());
    PrintOption_d(_LanternFaultDebounce, LanternFaultDebounce());
    PrintOption_d(_SlaveVoltageLow, SlaveVoltageLow());
    PrintOption_d(_SlaveVoltageHigh, SlaveVoltageHigh());
    PrintOption_d(_LightSensorScale, LightSensorScale());
    PrintOption_d(_SlavePowerUpDelay, SlavePowerUpDelay());
    PrintOption_d(_ColourBits, ColourBits());
    PrintOption_d(_IsResetLogAllowed, IsResetLogAllowed());
    PrintOption_d(_IsUpgradeAllowed, IsUpgradeAllowed());

    printf("\t%s '%s'\n", _Font, bFont.ToString().c_str());
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        if (bFont.GetBit(i))
        {
            printf("\t%s%d '%s'\n", _Font, i, fonts[i].FontName());
        }
    }
    printf("\t%s '%s'\n", _Conspicuity, bConspicuity.ToString().c_str());
    printf("\t%s '%s'\n", _Annulus, bAnnulus.ToString().c_str());
    printf("\t%s '%s'\n", _TxtFrmColour, bTxtFrmColour.ToString().c_str());
    printf("\t%s '%s'\n", _GfxFrmColour, bGfxFrmColour.ToString().c_str());
    printf("\t%s '%s'\n", _HrgFrmColour, bHrgFrmColour.ToString().c_str());
    
    printf("\tColour map:\n");
    for(int i=1;i<10,i++)
    {
        printf("\t%s '%s'\n", COLOUR_NAME[i], COLOUR_NAME[mappedColoursTable[i]]);
    }

    printf("\n---------------\n");
}

uint8_t UciProd::CharRows(int i)
{
    return (bFont.GetBit(i))?
        (pixelRows + fonts[i]->LineSpacing())/(fonts[i]->RowsPerCell()+fonts[i]->LineSpacing())
        : 0 ;
}

uint8_t UciProd::CharColumns(int i)
{
    return (bFont.GetBit(i))?
        (pixelColumns + fonts[i]->CharSpacing())/(fonts[i]->ColumnsPerCell()+fonts[i]->CharSpacing())
        : 0 ;
}


