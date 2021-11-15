#include <string>
#include <cstring>
#include <sys/stat.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/SerialPort.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <sign/Sign.h>
#include <uci/DbHelper.h>

extern const char *FirmwareMajorVer;
extern const char *FirmwareMinorVer;

using namespace Utils;

std::string StSignPort::ToString()
{
    char buf[64];
    if (com_ip < COMPORT_SIZE)
    {
        snprintf(buf, 255, "%s:%d", gSpConfig[com_ip].name, bps_port);
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
    signPort = nullptr;
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        fonts[i] = nullptr;
    }
}

UciProd::~UciProd()
{
    if (signPort != nullptr)
    {
        delete[] signPort;
    }
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        Font *v = fonts[i];
        if (v != nullptr)
        {
            delete fonts[i];
            for (int j = i; j < MAX_FONT + 1; j++)
            {
                if (v == fonts[j])
                {
                    fonts[j] = nullptr;
                }
            }
        }
    }
}

void UciProd::LoadConfig()
{
    PrintDbg(DBG_LOG, ">>> Loading 'prod'\n");
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciProd";
    SECTION = "ctrller_cfg";
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;

    tsiSp003Ver = GetIntFromStrz(uciSec, _TsiSp003Ver, TSISP003VER, TSISP003VER_SIZE);
    str = GetStr(uciSec, _MfcCode);
    if (strlen(str) == 6)
    {
        sprintf(mfcCode, "%s%s%s", str, FirmwareMajorVer, FirmwareMinorVer);
    }
    else
    {
        MyThrow("UciProd Error: MfcCode length should be 6");
    }

    prodType = GetIntFromStrz(uciSec, _ProdType, PRODTYPE, PRODTYPE_SIZE);
    numberOfSigns = GetInt(uciSec, _NumberOfSigns, 1, 16);
    numberOfGroups = GetInt(uciSec, _NumberOfGroups, 1, 16);
    groupCfg = new uint8_t[numberOfSigns];

    if (prodType == 0)
    { // VMS
        // VMS can only has 1 sign in 1 group
        // _GroupCfg ignored
        groupCfg = new uint8_t[numberOfSigns];
        for (int i = 0; i < numberOfSigns; i++)
        {
            groupCfg[i] = i + 1;
        }
        slaveRowsPerSign = GetInt(uciSec, _SlaveRowsPerSign, 1, 16);
        slaveColumnsPerSign = GetInt(uciSec, _SlaveColumnsPerSign, 1, 16);
    }
    else if (prodType == 1)
    { // ISLUS
        // ISLUS can only has 1 slave per Sign
        // _SlaveRowsPerSign/_SlaveColumnsPerSign ignored
        slaveRowsPerSign = 1;
        slaveColumnsPerSign = 1;

        str = GetStr(uciSec, _GroupCfg);
        cnt = Cnvt::GetIntArray(str, numberOfSigns, ibuf, 1, numberOfSigns);
        if (cnt == numberOfSigns)
        {
            for (cnt = 0; cnt < numberOfSigns; cnt++)
            {
                if (ibuf[cnt] == 0 || ibuf[cnt] > numberOfGroups)
                {
                    MyThrow("UciProd::GroupCfg Error: illegal group id : %d", ibuf[cnt]);
                }
                groupCfg[cnt] = ibuf[cnt];
            }
        }
        else
        {
            MyThrow("UciProd::GroupCfg Error: cnt!=%d", numberOfSigns);
        }
    }
    pixelRowsPerTile = GetInt(uciSec, _PixelRowsPerTile, 4, 255);
    pixelColumnsPerTile = GetInt(uciSec, _PixelColumnsPerTile, 8, 255);
    tileRowsPerSlave = GetInt(uciSec, _TileRowsPerSlave, 1, 32);
    tileColumnsPerSlave = GetInt(uciSec, _TileColumnsPerSlave, 1, 32);

    signPort = new struct StSignPort[numberOfSigns];
    for (int i = 1; i <= numberOfSigns; i++)
    {
        sprintf(cbuf, "%s%d", _Sign, i);
        str = GetStr(uciSec, cbuf);
        uint32_t x1, x2, x3, x4, x5;
        if (sscanf(str, "%u.%u.%u.%u:%u", &x1, &x2, &x3, &x4, &x5) == 5)
        {
            if (x1 != 0 && x1 < 256 && x2 < 256 && x3 < 256 && x4 != 0 && x4 < 255 && x5 > 1024 && x5 < 65536)
            {
                signPort[i - 1].com_ip = ((x1 * 0x100 + x2) * 0x100 + x3) * 0x100 + x4;
                signPort[i - 1].bps_port = x5;
                continue;
            }
        }
        else
        {
            for (cnt = 1; cnt < COMPORT_SIZE; cnt++)
            {
                if (memcmp(str, gSpConfig[cnt].name, 4) == 0)
                    break;
            }
            if (cnt < COMPORT_SIZE)
            {
                signPort[i - 1].com_ip = cnt;
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
                            signPort[i - 1].bps_port = x5;
                            continue;
                        }
                    }
                }
            }
        }
        MyThrow("UciProd Error: %s '%s'", cbuf, str);
    }

    slaveRqstInterval = GetInt(uciSec, _SlaveRqstInterval, 10, 5000);
    slaveRqstStTo = GetInt(uciSec, _SlaveRqstStTo, 10, 5000);
    slaveRqstExtTo = GetInt(uciSec, _SlaveRqstExtTo, 10, 5000);
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
    slaveVoltageDebounce = GetInt(uciSec, _SlaveVoltageDebounce, 1, 65535);

    if (slaveVoltageLow > slaveVoltageHigh)
    {
        MyThrow("UciProd Error: SlaveVoltageLow(%d) > SlaveVoltageHigh(%d)", slaveVoltageLow, slaveVoltageHigh);
    }
    lightSensorScale = GetInt(uciSec, _LightSensorScale, 1, 65535);

    slavePowerUpDelay = GetInt(uciSec, _SlavePowerUpDelay, 1, 255);

    driverMode = GetInt(uciSec, _DriverMode, 0, 1);
    dimmingAdjTime = GetInt(uciSec, _DimmingAdjTime, 5, 15);
    str = GetStr(uciSec, _ColourRatio);
    cnt = Cnvt::GetIntArray(str, 4, ibuf, 1, 255);
    if (cnt == 4)
    {
        for (cnt = 0; cnt < 4; cnt++)
        {
            colourRatio[cnt] = ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciProd::ColourRatio Error: cnt!=4");
    }
    str = GetStr(uciSec, _Dimming);
    cnt = Cnvt::GetIntArray(str, 16, ibuf, 1, 255);
    if (cnt == 16)
    {
        for (cnt = 0; cnt < 15; cnt++)
        {
            if (ibuf[cnt] > ibuf[cnt + 1])
            {
                MyThrow("UciProd::Dimming Error: Level:[%d]>[%d]", cnt, cnt + 1);
            }
        }
        for (cnt = 0; cnt < 16; cnt++)
        {
            dimming[cnt] = ibuf[cnt];
        }
    }
    else
    {
        MyThrow("UciProd::Dimming Error: cnt!=16");
    }

    str = GetStr(uciSec, _ColourLeds);
    cnt = strlen(str);
    if (cnt >= 1 && cnt <= 4)
    {
        strcpy(colourLeds, str);
    }
    else
    {
        MyThrow("UciProd Error: %s(%s)", _ColourLeds, str);
    }

    colourBits = GetInt(uciSec, _ColourBits, 1, 24);
    if (colourBits != 1 && colourBits != 4 && colourBits != 24)
    {
        MyThrow("UciProd Error: %s(%d) Only 1/4/24 allowed", _ColourBits, colourBits);
    }
    if ((strlen(colourLeds) == 1 && colourBits != 1) || (strlen(colourLeds) != 1 && colourBits == 1))
    {
        MyThrow("UciProd Error: %s(%d) not matched with %s('%s')", _ColourBits, colourBits, _ColourLeds, colourLeds);
    }

    isResetLogAllowed = GetInt(uciSec, _IsResetLogAllowed, 0, 1);
    isUpgradeAllowed = GetInt(uciSec, _IsUpgradeAllowed, 0, 1);

    ReadBool32(uciSec, _Font, bFont);
    if (!bFont.GetBit(0))
    {
        MyThrow("UciProd Error: Font : Default(0) is not enabled");
    }
    if ((bFont.Get() & 0xFFFFFFC0) != 0)
    {
        MyThrow("UciProd Error: Font : Only 0-5 allowed");
    }
    ReadBool32(uciSec, _Conspicuity, bConspicuity);
    if ((bConspicuity.Get() & 0xFFFFFFC0) != 0)
    {
        MyThrow("UciProd Error: Conspicuity : Only 0-5 allowed");
    }
    ReadBool32(uciSec, _Annulus, bAnnulus);
    if ((bAnnulus.Get() & 0xFFFFFFF8) != 0)
    {
        MyThrow("UciProd Error: Annulus : Only 0-2 allowed");
    }
    ReadBool32(uciSec, _TxtFrmColour, bTxtFrmColour);
    ReadBool32(uciSec, _GfxFrmColour, bGfxFrmColour);
    ReadBool32(uciSec, _HrgFrmColour, bHrgFrmColour);
    // check colourLeds & b___FrmColour

    // Font0 'P5X7'
    // Font1 'F5X7'
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        fonts[i] = nullptr;
        if (bFont.GetBit(i))
        {
            sprintf(cbuf, "%s%d", _Font, i);
            str = GetStr(uciSec, cbuf);
            if (strlen(str) > 15)
            {
                MyThrow("UciProd Error: %s '%s'(file name length too long >15)", cbuf, str);
            }
            int j;
            for (j = 0; j < i; j++)
            {
                if (fonts[j] != nullptr && strcmp(str, fonts[j]->FontName()) == 0)
                {
                    break;
                }
            }
            if (j < i)
            { // found a loaded font
                fonts[i] = fonts[j];
            }
            else
            { // new font to load
                fonts[i] = new Font(str);
            }
        }
    }

    mappedColoursTable[0] = 0;
    for (int i = 1; i < 10; i++)
    {
        str = GetStr(uciSec, COLOUR_NAME[i]);
        for (cnt = 1; cnt < 10; cnt++)
        {
            if (strcmp(str, COLOUR_NAME[cnt]) == 0)
            {
                mappedColoursTable[i] = cnt;
                break;
            }
        }
        if (cnt == 10)
        {
            MyThrow("UciProd Error: colour map %s undefined", COLOUR_NAME[i]);
        }
    }

    Close();
    Dump();

    Slave::numberOfTiles = tileRowsPerSlave * tileColumnsPerSlave;
    Slave::numberOfColours = strlen(colourLeds);

    pixelRows = (uint16_t)pixelRowsPerTile * tileRowsPerSlave * slaveRowsPerSign;
    pixelColumns = (uint16_t)pixelColumnsPerTile * tileColumnsPerSlave * slaveColumnsPerSign;
    pixels = (uint32_t)pixelRows * pixelColumns;

    gfx1FrmLen = 0;
    gfx4FrmLen = 0;
    gfx24FrmLen = 0;
    int sesrtype = mfcCode[5] - '0';
    if (sesrtype == 0)
    {
        extStsRplSignType = SESR_SIGN_TYPE::TEXT;
        configRplSignType = SCR_SIGN_TYPE::TEXT;
        maxFrmLen = 255;
    }
    else if (sesrtype == 1 || sesrtype == 2)
    {
        extStsRplSignType = (sesrtype == 1) ? SESR_SIGN_TYPE::GFX : SESR_SIGN_TYPE::ADVGFX;
        switch (colourBits)
        {
        case 1:
            configRplSignType = SCR_SIGN_TYPE::GFXMONO;
            maxFrmLen = (pixels + 7) / 8;
            gfx1FrmLen = maxFrmLen;
            break;
        case 4:
            configRplSignType = SCR_SIGN_TYPE::GFXMULTI;
            maxFrmLen = (pixels + 1) / 2;
            gfx1FrmLen = (pixels + 7) / 8;
            gfx4FrmLen = maxFrmLen;
            break;
        case 24:
            configRplSignType = SCR_SIGN_TYPE::GFXRGB24;
            maxFrmLen = pixels * 3;
            gfx1FrmLen = (pixels + 7) / 8;
            gfx4FrmLen = (pixels + 1) / 2;
            gfx24FrmLen = maxFrmLen;
            break;
        default:
            MyThrow("Unknown ColourBits in UciProd");
            break;
        }
    }
    else
    {
        MyThrow("Unknown extStSignType:%d(MfcCode error?)", sesrtype);
    }
}

void UciProd::Dump()
{
    PrintDash();
    printf("%s/%s.%s\n", PATH, PACKAGE, SECTION);

    PrintOption_str(_TsiSp003Ver, TSISP003VER[TsiSp003Ver()]);
    PrintOption_str(_ProdType, PRODTYPE[ProdType()]);
    PrintOption_str(_MfcCode, MfcCode());

    PrintOption_d(_NumberOfSigns, NumberOfSigns());
    for (int i = 1; i <= NumberOfSigns(); i++)
    {
        printf("\t%s%d \t'%s'\n", _Sign, i, SignPort(i)->ToString().c_str());
    }

    PrintOption_d(_NumberOfGroups, NumberOfGroups());
    printf("\t%s \t'%u", _GroupCfg, groupCfg[0]);
    for (int i = 1; i < numberOfSigns; i++)
    {
        printf(",%u", groupCfg[i]);
    }
    printf("'\n");

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
    PrintOption_d(_SlaveVoltageDebounce, SlaveVoltageDebounce());
    PrintOption_d(_LightSensorScale, LightSensorScale());
    PrintOption_d(_SlavePowerUpDelay, SlavePowerUpDelay());
    PrintOption_d(_DriverMode, DriverMode());
    uint8_t *p;
    p = ColourRatio();
    printf("\t%s\t'%u", _ColourRatio, *p++);
    for (int i = 1; i < 4; i++)
    {
        printf(",%u", *p++);
    }
    printf("'\n");
    p = Dimming();
    printf("\t%s\t'%u", _Dimming, *p++);
    for (int i = 1; i < 16; i++)
    {
        printf(",%u", *p++);
    }
    printf("'\n");
    PrintOption_d(_DimmingAdjTime, DimmingAdjTime());

    PrintOption_d(_ColourBits, ColourBits());
    PrintOption_str(_ColourLeds, ColourLeds());

    PrintOption_str(_TxtFrmColour, bTxtFrmColour.ToString().c_str());
    PrintOption_str(_GfxFrmColour, bGfxFrmColour.ToString().c_str());
    PrintOption_str(_HrgFrmColour, bHrgFrmColour.ToString().c_str());

    printf("\tColour map:\n");
    for (int i = 1; i < 10; i++)
    {
        PrintOption_str(COLOUR_NAME[i], COLOUR_NAME[mappedColoursTable[i]]);
    }

    PrintOption_str(_Font, bFont.ToString().c_str());
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        if (bFont.GetBit(i))
        {
            printf("\t%s%d \t'%s'\n", _Font, i, Fonts(i)->FontName());
        }
    }
    PrintOption_str(_Conspicuity, bConspicuity.ToString().c_str());
    PrintOption_str(_Annulus, bAnnulus.ToString().c_str());

    PrintOption_d(_IsResetLogAllowed, IsResetLogAllowed());
    PrintOption_d(_IsUpgradeAllowed, IsUpgradeAllowed());
}

uint8_t UciProd::CharRows(int i)
{
    return (bFont.GetBit(i)) ? (pixelRows + fonts[i]->LineSpacing()) / (fonts[i]->RowsPerCell() + fonts[i]->LineSpacing())
                             : 0;
}

uint8_t UciProd::CharColumns(int i)
{
    return (bFont.GetBit(i)) ? (pixelColumns + fonts[i]->CharSpacing()) / (fonts[i]->ColumnsPerCell() + fonts[i]->CharSpacing())
                             : 0;
}
