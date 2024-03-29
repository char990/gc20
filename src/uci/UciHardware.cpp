#include <string>
#include <cstring>
#include <sys/stat.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/SerialPort.h>
#include <module/MyDbg.h>
#include <module/Utils.h>
#include <sign/Sign.h>
#include <uci/DbHelper.h>
#include <sign/FrameImage.h>

extern const char *FirmwareVer;

using namespace Utils;
using namespace std;

int SignCfg::bps_port = 0;

UciHardware::UciHardware()
{
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        fonts[i] = nullptr;
    }
}

UciHardware::~UciHardware()
{
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

void UciHardware::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciHardware";
    DebugLog(">>> Loading '%s/%s'", PATH, PACKAGE);
    char cbuf[16];
    int ibuf[16];
    const char *str;
    int cnt;
    Open();

    SECTION = _SectionCtrller;
    struct uci_section *uciSec = GetSection(SECTION);

    tcpServerTMC = GetInt(uciSec, _TcpServerTMC, 1, 8);
    tcpTimeout = GetInt(uciSec, _TcpTimeout, 60, 0xFFFF);

    tsiSp003Ver = GetIndexFromStrz(uciSec, _TsiSp003Ver, TSISP003VER, TSISP003VER_SIZE);
    str = GetStr(uciSec, _MfcCode);
    if (strlen(str) == 6)
    {
        memcpy(mfcCode, str, 6);
        memcpy(mfcCode + 6, FirmwareVer, 4);
        mfcCode[10] = '\0';
    }
    else
    {
        ThrowError(_MfcCode, "Should be 6 bytes");
    }

    auto achrs = GetStr(uciSec, _AllowedAscii, false);
    if (achrs != nullptr)
    {
        while (char c = *achrs)
        {
            if (c >= ' ' && c < 127)
            {
                allowedAscii.SetBit(c);
            }
            else
            {
                ThrowError(_MfcCode, "Invalid Ascii");
            }
            achrs++;
        }
        allowedAscii.SetBit('"');
        allowedAscii.SetBit('\'');
    }
    else
    {
        for (char c = ' '; c < 127; c++)
        {
            allowedAscii.SetBit(c);
        }
    }

    testTMC = GetInt(uciSec, _TestTMC, 1, 255);
    testSlave = GetInt(uciSec, _TestSlave, 1, 255);

    pixelRows = GetInt(uciSec, _PixelRows, 8, 4096);
    pixelColumns = GetInt(uciSec, _PixelColumns, 8, 4096);
    tilesPerSlave = GetInt(uciSec, _TilesPerSlave, 1, 255);
    slavesPerSign = GetInt(uciSec, _SlavesPerSign, 1, 16);
    coreOffsetX = GetInt(uciSec, _CoreOffsetX, 0, pixelColumns);
    coreOffsetY = GetInt(uciSec, _CoreOffsetY, 0, pixelRows);
    str = GetStr(uciSec, _DisplayArea);
    cnt = Cnvt::GetIntArray(str, 4, ibuf, 0, 65535);
    if (cnt == 4 && ibuf[0] < ibuf[2] && ibuf[1] < ibuf[3] && ibuf[2] < pixelColumns && ibuf[3] < pixelRows)
    {
        for (int i = 0; i < 4; i++)
        {
            displayArea[i] = ibuf[i];
        }
    }
    else
    {
        ThrowError(cbuf, str);
    }


    numberOfSigns = GetInt(uciSec, _NumberOfSigns, 1, 12);
    numberOfGroups = GetInt(uciSec, _NumberOfGroups, 1, 4);

    int _prodT = GetIndexFromStrz(uciSec, _ProdType, PRODTYPE, PRODTYPE_SIZE);
    if (_prodT == 0)
    {
        prodType = PRODUCT::VMS;
    }
    else if (_prodT == 1)
    {
        prodType = PRODUCT::ISLUS;
        ReadBits(uciSec, _IslusSpFrm, bIslusSpFrm);
    }
    else
    {
        ThrowError(_ProdType, "Unknown prod type");
    }

    slaveRqstInterval = GetInt(uciSec, _SlaveRqstInterval, 10, 5000);
    slaveRqstStTo = GetInt(uciSec, _SlaveRqstStTo, 10, 5000);
    slaveRqstExtTo = GetInt(uciSec, _SlaveRqstExtTo, 10, 5000);
    if (slaveRqstStTo > slaveRqstInterval)
    {
        ThrowError(_SlaveRqstStTo, "SlaveRqstStTo should greater than SlaveRqstInterval");
    }
    if (slaveRqstExtTo > slaveRqstInterval)
    {
        ThrowError(_SlaveRqstExtTo, "slaveRqstExtTo should greater than SlaveRqstInterval");
    }
    slaveSetStFrmDly = GetInt(uciSec, _SlaveSetStFrmDly, 10, 1000);
    slaveDispDly = GetInt(uciSec, _SlaveDispDly, 10, 1000);
    slaveCmdDly = GetInt(uciSec, _SlaveCmdDly, 10, 1000);
    if (slaveCmdDly > slaveSetStFrmDly)
    {
        ThrowError(_SlaveCmdDly, "SlaveCmdDly should greater than SlaveSetStFrmDly");
    }
    if (slaveCmdDly > slaveDispDly)
    {
        ThrowError(_SlaveCmdDly, "SlaveCmdDly should greater than SlaveDispDly");
    }

    overTempDebounce = GetInt(uciSec, _OverTempDebounce, 3, 65535);
    mledFaultDebounce = GetInt(uciSec, _MLedFaultDebounce, 3, 65535);
    sledFaultDebounce = GetInt(uciSec, _SLedFaultDebounce, 3, 65535);
    if (mledFaultDebounce >= sledFaultDebounce)
    {
        ThrowError(_MLedFaultDebounce, "SLedFaultDebounce should greater than MLedFaultDebounce");
    }
    selftestDebounce = GetInt(uciSec, _SelftestDebounce, 3, 65535);
    driverFaultDebounce = GetInt(uciSec, _DriverFaultDebounce, 3, 65535);
    if (driverFaultDebounce >= mledFaultDebounce)
    {
        ThrowError(_MLedFaultDebounce, "MLedFaultDebounce should greater than DriverFaultDebounce");
    }
    offlineDebounce = GetInt(uciSec, _OfflineDebounce, 3, 65535);
    lightSensorFaultDebounce = GetInt(uciSec, _LightSensorFaultDebounce, 60, 65535);
    lanternFaultDebounce = GetInt(uciSec, _LanternFaultDebounce, 3, 65535);

    slaveVoltageLow = GetInt(uciSec, _SlaveVoltageLow, 1, 65535);
    slaveVoltageHigh = GetInt(uciSec, _SlaveVoltageHigh, 1, 65535);
    slaveVoltageDebounce = GetInt(uciSec, _SlaveVoltageDebounce, 3, 65535);
    if (slaveVoltageLow >= slaveVoltageHigh)
    {
        ThrowError(_SlaveVoltageHigh, "SlaveVoltageHigh should greater than SlaveVoltageLow");
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
        ThrowError(_ColourRatio, "cnt!=4");
    }
    str = GetStr(uciSec, _Dimming);
    cnt = Cnvt::GetIntArray(str, 16, ibuf, 1, 255);
    if (cnt == 16)
    {
        for (cnt = 0; cnt < 15; cnt++)
        {
            if (ibuf[cnt] > ibuf[cnt + 1])
            {
                ThrowError(_Dimming, "Value should increase with level");
            }
        }
        for (cnt = 0; cnt < 16; cnt++)
        {
            dimming[cnt] = ibuf[cnt];
        }
    }
    else
    {
        ThrowError(_Dimming, "cnt!=16");
    }

    str = GetStr(uciSec, _ColourLeds);
    cnt = strlen(str);
    if (cnt >= 1 && cnt <= 4)
    {
        strcpy(colourLeds, str);
    }
    else
    {
        ThrowError(_ColourLeds, str);
    }

    colourBits = GetInt(uciSec, _ColourBits, 1, 24);
    if (colourBits != 1 && colourBits != 4) // && colourBits != 24)
    {
        ThrowError(_ColourBits, "Only 1/4 allowed");
    }
    if (strlen(colourLeds) > colourBits)
    {
        ThrowError(_ColourBits, "Not matched with ColourLeds");
    }

    hasLanterns = GetInt(uciSec, _HasLanterns, 0, 1);
    FrameImage::hasLanterns = HasLanterns();
    isResetLogAllowed = GetInt(uciSec, _IsResetLogAllowed, 0, 1);
    isUpgradeAllowed = GetInt(uciSec, _IsUpgradeAllowed, 0, 1);
    loadLastDisp = GetInt(uciSec, _LoadLastDisp, 0, 1);

    ReadBits(uciSec, _Font, bFont);
    if (!bFont.GetBit(0))
    {
        ThrowError(_Font, "Default(0) is not enabled");
    }
    if (bFont.GetMaxBit() > 5)
    {
        ThrowError(_Font, "Only 0-5 allowed");
    }
    ReadBits(uciSec, _Conspicuity, bConspicuity);
    if (!bConspicuity.GetBit(0))
    {
        ThrowError(_Conspicuity, "OFF(0) is not enabled");
    }
    if (bConspicuity.GetMaxBit() > 5)
    {
        ThrowError(_Conspicuity, "Only 0-5 allowed");
    }
    ReadBits(uciSec, _Annulus, bAnnulus);
    if (!bAnnulus.GetBit(0))
    {
        ThrowError(_Annulus, "OFF(0) is not enabled");
    }
    if (bAnnulus.GetMaxBit() > 2)
    {
        ThrowError(_Annulus, "Only 0-5 allowed");
    }
    ReadBits(uciSec, _TxtFrmColour, bTxtFrmColour);
    ReadBits(uciSec, _GfxFrmColour, bGfxFrmColour);
    ReadBits(uciSec, _HrgFrmColour, bHrgFrmColour);

    ReadBits(uciSec, _SimSlaves, bSimSlaves, false);

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
            if (strlen(str) > 8)
            {
                ThrowError(cbuf, "file name is too long > 8");
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

    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        str = GetStr(uciSec, FrameColour::COLOUR_NAME[i]);
        for (cnt = 1; cnt < MONO_COLOUR_NAME_SIZE; cnt++)
        {
            if (strcmp(str, FrameColour::COLOUR_NAME[cnt]) == 0)
            {
                mappedColoursTable[i] = cnt;
                break;
            }
        }
        if (cnt == MONO_COLOUR_NAME_SIZE)
        {
            ThrowError(FrameColour::COLOUR_NAME[i], "colour map is undefined");
        }
    }

    try
    {
        monitoringPort = GetIndexFromStrz(uciSec, _MonitoringPort, COM_NAME, COMPORT_SIZE);
    }
    catch (...)
    {
    }
    if (monitoringPort >= 0)
    {
        monitoringBps = GetInt(uciSec, _MonitoringBps, ALLOWEDBPS, STANDARDBPS_SIZE);
    }

    /********************* SignX ******************/
    signCfg.resize(numberOfSigns);
    SignCfg::bps_port = GetInt(uciSec, _SlaveBpsPort, ALLOWEDBPS, STANDARDBPS_SIZE, false);
    if (SignCfg::bps_port == 0)
    { // IP:Port
        SignCfg::bps_port = GetInt(uciSec, _SlaveBpsPort, INT_MIN, INT_MAX);
        int port = -SignCfg::bps_port;
        if (port < 1024 || port > 0xFFFF)
        {
            ThrowError(_SlaveBpsPort, "Port should be '-'1024~65535");
        }
    }
    char signx[16];
    SECTION = signx;
    for (int i = 0; i < numberOfSigns; i++)
    {
        auto &_sign = signCfg.at(i);
        sprintf(signx, "%s%d", _SectionSign, i + 1);
        uciSec = GetSection(SECTION);
        _sign.groupId = GetInt(uciSec, _GroupId, 1, numberOfGroups);
        ReadBits(uciSec, _RejectFrms, _sign.rejectFrms, false);
        if (SignCfg::bps_port > 0)
        { // COM
            _sign.com_ip = GetIndexFromStrz(uciSec, _COM, COM_NAME, COMPORT_SIZE);
            if (_sign.com_ip == monitoringPort)
            {
                ThrowError(_COM, "Assigned to MonitoringPort");
            }
        }
        else
        { // IP
            str = GetStr(uciSec, _IP);
            uint32_t x1, x2, x3, x4;
            if (sscanf(str, "%u.%u.%u.%u", &x1, &x2, &x3, &x4) == 4)
            {
                if (x1 != 0 && x1 < 256 && x2 < 256 && x3 < 256 && x4 != 0 && x4 < 255)
                {
                    _sign.com_ip = ((x1 * 0x100 + x2) * 0x100 + x3) * 0x100 + x4;
                }
                else
                {
                    ThrowError(_IP, "Invalid IP");
                }
            }
        }
    }

    if (SignCfg::bps_port > 0)
    { // check COM
        for (int g = 1; g <= numberOfGroups; g++)
        {
            int com = -1;
            for (int i = 0; i < numberOfSigns; i++)
            {
                auto &_sign = signCfg.at(i);
                if (_sign.groupId == g)
                {
                    if (com < 0)
                    {
                        com = _sign.com_ip;
                    }
                    else
                    {
                        if (com != _sign.com_ip)
                        {
                            throw invalid_argument(StrFn::PrintfStr("%s.%s_.%s Error: Signs in Group[%d] should have same COM",
                                                                    PACKAGE, _SectionSign, _COM, g));
                        }
                    }
                }
            }
        }
    }

    if (prodType == PRODUCT::VMS)
    {
        // VMS should configured as 1 sign per group
        for (int i = 0; i < numberOfSigns; i++)
        {
            if (signCfg.at(i).groupId != (i + 1))
            {
                ThrowError("GroupCfg", "1 group 1 sign for VMS");
            }
        }
    }
    else if (prodType == PRODUCT::ISLUS)
    {
        // ISLUS should configured as 1 slave per Sign
        if (slavesPerSign != 1)
        {
            ThrowError(_SlavesPerSign, "Should be 1 for ISLUS");
        }
    }

    Close();
    Dump();

    int cblen = strlen(colourLeds);
    for (int i = 0; i < cblen; i++)
    {
        for (cnt = 1; cnt < MONO_COLOUR_NAME_SIZE; cnt++)
        {
            if (colourLeds[i] == FrameColour::COLOUR_NAME[cnt][0])
            { // matched with first letter of colour
                mappedColoursBitTable[cnt] = 1 << i;
                break;
            }
        }
    }

    Slave::numberOfTiles = tilesPerSlave;
    Slave::numberOfColours = strlen(colourLeds);

    pixels = (uint32_t)pixelRows * pixelColumns;
    pixelsPerSign = pixels / numberOfSigns;
    pixelsPerSlave = pixelsPerSign / slavesPerSign;

    gfx1CoreLen = 0;
    gfx4CoreLen = 0;
    gfx24CoreLen = 0;
    int sesrtype = mfcCode[5] - '0';
    if (sesrtype == 0)
    {
        extStsRplSignType = SESR_SIGN_TYPE::TEXT;
        configRplSignType = SCR_SIGN_TYPE::TEXT;
        maxCoreLen = 255;
    }
    else if (sesrtype == 1 || sesrtype == 2)
    {
        extStsRplSignType = (sesrtype == 1) ? SESR_SIGN_TYPE::GFX : SESR_SIGN_TYPE::ADVGFX;
        switch (colourBits)
        {
        case 1:
            configRplSignType = SCR_SIGN_TYPE::GFXMONO;
            gfx1CoreLen = (pixels + 7) / 8;
            maxCoreLen = gfx1CoreLen;
            break;
        case 4:
            configRplSignType = SCR_SIGN_TYPE::GFXMULTI;
            gfx1CoreLen = (pixels + 7) / 8;
            gfx4CoreLen = (pixels + 1) / 2;
            maxCoreLen = gfx4CoreLen;
            break;
        case 24:
            configRplSignType = SCR_SIGN_TYPE::GFXRGB24;
            gfx1CoreLen = (pixels + 7) / 8;
            gfx4CoreLen = (pixels + 1) / 2;
            gfx24CoreLen = pixels * 3;
            maxCoreLen = gfx24CoreLen;
            break;
        }
    }
    else
    {
        throw invalid_argument(StrFn::PrintfStr("Unknown extStSignType:%d(MfcCode[5] error?)", sesrtype));
    }
}

void UciHardware::Dump()
{
    PrintDash('<');
    printf("%s/%s.%s\n", PATH, PACKAGE, _SectionCtrller);

    PrintOption_d(_TcpServerTMC, TcpServerTMC());
    PrintOption_d(_TcpTimeout, TcpTimeout());

    PrintOption_str(_TsiSp003Ver, TSISP003VER[TsiSp003Ver()]);
    PrintOption_str(_ProdType, PRODTYPE[static_cast<int>(ProdType())]);
    PrintOption_str(_MfcCode, MfcCode());

    printf("\t%s\t'", _AllowedAscii);
    for (int i = 0; i < 128; i++)
    {
        if (allowedAscii.GetBit(i))
        {
            printf("%c", i);
        }
    }
    printf("'\n");

    PrintOption_d(_TestTMC, TestTMC());
    PrintOption_d(_TestSlave, TestSlave());

    PrintOption_d(_PixelRows, PixelRows());
    PrintOption_d(_PixelColumns, PixelColumns());
    PrintOption_d(_TilesPerSlave, TilesPerSlave());
    PrintOption_d(_SlavesPerSign, SlavesPerSign());
    PrintOption_d(_CoreOffsetX, CoreOffsetX());
    PrintOption_d(_CoreOffsetY, CoreOffsetY());
    printf("\t%s\t'%u", _DisplayArea, displayArea[0]);
    for (int i = 1; i < 4; i++)
    {
        printf(",%u", displayArea[i]);
    }
    printf("'\n");

    PrintOption_d(_SlaveRqstInterval, SlaveRqstInterval());
    PrintOption_d(_SlaveRqstStTo, SlaveRqstStTo());
    PrintOption_d(_SlaveRqstExtTo, SlaveRqstExtTo());
    PrintOption_d(_SlaveSetStFrmDly, SlaveSetStFrmDly());
    PrintOption_d(_SlaveDispDly, SlaveDispDly());
    PrintOption_d(_SlaveCmdDly, SlaveCmdDly());
    PrintOption_d(_DriverFaultDebounce, DriverFaultDebounce());
    PrintOption_d(_MLedFaultDebounce, MLedFaultDebounce());
    PrintOption_d(_SLedFaultDebounce, SLedFaultDebounce());
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
    for (int i = 0; i < MONO_COLOUR_NAME_SIZE; i++)
    {
        PrintOption_str(FrameColour::COLOUR_NAME[i], FrameColour::COLOUR_NAME[mappedColoursTable[i]]);
    }

    PrintOption_str(_Font, bFont.ToString().c_str());
    for (int i = 0; i < MAX_FONT + 1; i++)
    {
        if (bFont.GetBit(i))
        {
            printf("\t%s%d \t'%s'\n", _Font, i, Fonts(i)->FontName());
        }
    }
    PrintOption_d(_HasLanterns, HasLanterns());
    PrintOption_str(_Conspicuity, bConspicuity.ToString().c_str());
    PrintOption_str(_Annulus, bAnnulus.ToString().c_str());

    PrintOption_d(_IsResetLogAllowed, IsResetLogAllowed());
    PrintOption_d(_IsUpgradeAllowed, IsUpgradeAllowed());
    PrintOption_d(_LoadLastDisp, LoadLastDisp());

    if (prodType == PRODUCT::ISLUS)
    {
        PrintOption_str(_IslusSpFrm, bIslusSpFrm.ToString().c_str());
    }

    if (monitoringPort >= 0)
    {
        PrintOption_str(_MonitoringPort, COM_NAME[monitoringPort]);
        PrintOption_d(_MonitoringBps, MonitoringBps());
    }

    PrintOption_d(_SlaveBpsPort, SignCfg::bps_port);
    PrintOption_d(_NumberOfGroups, NumberOfGroups());
    PrintOption_d(_NumberOfSigns, NumberOfSigns());

    char ipbuf[16];
    for (int i = 1; i <= NumberOfSigns(); i++)
    {
        printf("\n%s/%s.%s%d\n", PATH, PACKAGE, _SectionSign, i);
        auto &cfg = GetSignCfg(i);
        PrintOption_d(_GroupId, cfg.groupId);
        auto com_ip = cfg.com_ip;
        if (com_ip < COMPORT_SIZE)
        {
            PrintOption_str(_COM, COM_NAME[com_ip]);
        }
        else
        {
            uint8_t *p = (uint8_t *)(&com_ip);
            sprintf(ipbuf, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
            PrintOption_str(_IP, ipbuf);
        }
        PrintOption_str(_RejectFrms, cfg.rejectFrms.ToString().c_str());
    }

    PrintDash('>');
}

uint8_t UciHardware::CharRows(int i)
{
    return (bFont.GetBit(i)) ? (pixelRows + fonts[i]->LineSpacing()) / (fonts[i]->RowsPerCell() + fonts[i]->LineSpacing())
                             : 0;
}

uint8_t UciHardware::CharColumns(int i)
{
    return (bFont.GetBit(i)) ? (pixelColumns + fonts[i]->CharSpacing()) / (fonts[i]->ColumnsPerCell() + fonts[i]->CharSpacing())
                             : 0;
}

uint8_t UciHardware::GetColourXbit(uint8_t c)
{
    auto c1 = (c > 9) ? mappedColoursTable[0] : mappedColoursTable[c];
    return mappedColoursBitTable[c1];
}
