#pragma once

#include <string>
#include <vector>

#include <tsisp003/TsiSp003Const.h>
#include <uci/UciCfg.h>
#include <module/Utils.h>
#include <uci/Font.h>

class SignCfg
{
public:
    uint8_t groupId;
    uint32_t com_ip;
    static int bps_port;
    Utils::Bits rejectFrms{256};
};

class UciHardware : public UciCfg
{
public:
    UciHardware();
    ~UciHardware();

    void LoadConfig() override;
    void Dump() override;

    // getter
    uint8_t TcpServerTMC() { return tcpServerTMC; };
    uint8_t TestTMC() { return testTMC; };
    uint8_t TestSlave() { return testSlave; };
    char *MfcCode() { return &mfcCode[0]; };

    Font *Fonts(int i) { return fonts[i]; };
    SignCfg &GetSignCfg(uint8_t id) { return signCfg.at(id - 1); };
    uint8_t *MappedColoursTable() { return &mappedColoursTable[0]; };
    uint8_t GetMappedColour(uint8_t c)
    {
        return (c > 9) ? mappedColoursTable[0] : mappedColoursTable[c];
    }

    uint16_t SlaveRqstInterval() { return slaveRqstInterval; };
    uint16_t SlaveRqstStTo() { return slaveRqstStTo; };
    uint16_t SlaveRqstExtTo() { return slaveRqstExtTo; };
    uint16_t SlaveSetStFrmDly() { return slaveSetStFrmDly; };
    uint16_t SlaveDispDly() { return slaveDispDly; };
    uint16_t SlaveCmdDly() { return slaveCmdDly; };

    uint16_t DriverFaultDebounce() { return driverFaultDebounce; };
    uint16_t MLedFaultDebounce() { return mledFaultDebounce; };
    uint16_t SLedFaultDebounce() { return sledFaultDebounce; };
    uint16_t OverTempDebounce() { return overTempDebounce; };
    uint16_t SelftestDebounce() { return selftestDebounce; };
    uint16_t OfflineDebounce() { return offlineDebounce; };
    uint16_t LightSensorFaultDebounce() { return lightSensorFaultDebounce; };
    uint16_t LanternFaultDebounce() { return lanternFaultDebounce; };
    uint16_t SlaveVoltageLow() { return slaveVoltageLow; };
    uint16_t SlaveVoltageHigh() { return slaveVoltageHigh; };
    uint16_t SlaveVoltageDebounce() { return slaveVoltageDebounce; };
    uint16_t LightSensorScale() { return lightSensorScale; };
    uint16_t TcpTimeout() { return tcpTimeout; }; // seconds

    uint8_t TsiSp003Ver() { return tsiSp003Ver; };
    PRODUCT ProdType() { return prodType; };
    uint8_t SlavePowerUpDelay() { return slavePowerUpDelay; };
    uint8_t ColourBits() { return colourBits; };
    uint8_t NumberOfSigns() { return numberOfSigns; };
    uint8_t NumberOfGroups() { return numberOfGroups; };
    uint8_t GetGroupIdOfSign(uint8_t signId) { return signCfg.at(signId - 1).groupId; };
    uint16_t PixelRows() { return pixelRows; };
    uint16_t PixelColumns() { return pixelColumns; };
    uint8_t TilesPerSlave() { return tilesPerSlave; };
    uint8_t SlavesPerSign() { return slavesPerSign; };
    uint16_t CoreOffsetX() { return coreOffsetX; };
    uint16_t CoreOffsetY() { return coreOffsetY; };
    uint16_t *DisplayArea() { return displayArea; };

    uint8_t *Dimming() { return &dimming[0]; };
    uint8_t Dimming(uint8_t lvl) { return dimming[lvl - 1]; };
    uint8_t *ColourRatio() { return &colourRatio[0]; };
    uint8_t DriverMode() { return driverMode; };
    uint8_t DimmingAdjTime() { return dimmingAdjTime; };
    char *ColourLeds() { return colourLeds; };
    uint8_t GetColourXbit(uint8_t colour);

    bool IsResetLogAllowed() { return isResetLogAllowed != 0; };
    bool IsUpgradeAllowed() { return isUpgradeAllowed != 0; };

    bool IsFont(int i) { return bFont.GetBit(i); };
    bool IsConspicuity(int i) { return bConspicuity.GetBit(i); };
    bool IsAnnulus(int i) { return bAnnulus.GetBit(i); };
    bool IsTxtFrmColourValid(int i) { return bTxtFrmColour.GetBit(i); };
    bool IsGfxFrmColourValid(int i) { return bGfxFrmColour.GetBit(i); };
    bool IsHrgFrmColourValid(int i) { return bHrgFrmColour.GetBit(i); };

    bool IsIslusSpFrm(int i) { return bIslusSpFrm.GetBit(i); };

    bool IsSimSlave(int i) { return bSimSlaves.GetBit(i); };

    bool HasLanterns() {return hasLanterns!=0;};

    uint8_t MaxConspicuity() { return bConspicuity.GetMaxBit(); };
    uint8_t MaxFont() { return bFont.GetMaxBit(); };
    uint8_t MaxAnnulus() { return bAnnulus.GetMaxBit(); };

    // configurations calculated from other configurations
    uint8_t CharRows(int fontX);
    uint8_t CharColumns(int fontX);
    uint32_t Pixels() { return pixels; };
    uint32_t PixelsPerSign(){ return pixelsPerSign; };
    uint32_t PixelsPerSlave(){return pixelsPerSlave; };
    SESR_SIGN_TYPE ExtStsRplSignType() { return extStsRplSignType; };
    SCR_SIGN_TYPE ConfigRplSignType() { return configRplSignType; };
    int MaxCoreLen() { return maxCoreLen; };

    int MinTxtFrmLen() { return 1; };
    int MaxTxtFrmLen() { return 255; };
    int Gfx1CoreLen() { return gfx1CoreLen; };   // bytes
    int Gfx4CoreLen() { return gfx4CoreLen; };   // bytes
    int Gfx24CoreLen() { return gfx24CoreLen; }; // bytes

    uint8_t LoadLastDisp() { return loadLastDisp; };

    int MonitoringPort() { return monitoringPort; };
    int MonitoringBps() { return monitoringBps; };

    bool IsAsciiAllowed(char c) { return allowedAscii.GetBit(c); };

private:
    ///  ---------- section -----------
    const char *_SectionCtrller = "ctrller_cfg";
    const char *_SectionSign = "Sign";

    ///  ---------- option -----------
    // string
    const char *_MfcCode = "MfcCode";
    const char *_ProdType = "ProdType";
    const char *_AllowedAscii = "AllowedAscii";

    // int array
    const char *_Font = "Font";
    const char *_Conspicuity = "Conspicuity";
    const char *_Annulus = "Annulus";
    const char *_TxtFrmColour = "TxtFrmColour";
    const char *_GfxFrmColour = "GfxFrmColour";
    const char *_HrgFrmColour = "HrgFrmColour";
    // const char *_GroupCfg = "GroupCfg";
    const char *_DisplayArea = "DisplayArea"; // "left,top,right,bottom"

    /// int
    const char *_SlaveRqstInterval = "SlaveRqstInterval";
    const char *_SlaveRqstStTo = "SlaveRqstStTo";
    const char *_SlaveRqstExtTo = "SlaveRqstExtTo";
    const char *_SlaveSetStFrmDly = "SlaveSetStFrmDly";
    const char *_SlaveDispDly = "SlaveDispDly";
    const char *_SlaveCmdDly = "SlaveCmdDly";
    const char *_SlaveBpsPort = "SlaveBpsPort";

    /// uint16_t
    const char *_DriverFaultDebounce = "DriverFaultDebounce";
    const char *_MLedFaultDebounce = "MLedFaultDebounce";
    const char *_SLedFaultDebounce = "SLedFaultDebounce";
    const char *_OverTempDebounce = "OverTempDebounce";
    const char *_SelftestDebounce = "SelftestDebounce";
    const char *_OfflineDebounce = "OfflineDebounce";
    const char *_LightSensorFaultDebounce = "LightSensorFaultDebounce";
    const char *_LanternFaultDebounce = "LanternFaultDebounce";
    const char *_SlaveVoltageLow = "SlaveVoltageLow";
    const char *_SlaveVoltageHigh = "SlaveVoltageHigh";
    const char *_SlaveVoltageDebounce = "SlaveVoltageDebounce";
    const char *_PixelRows = "PixelRows";
    const char *_PixelColumns = "PixelColumns";
    const char *_CoreOffsetX = "CoreOffsetX";
    const char *_CoreOffsetY = "CoreOffsetY";
    const char *_TcpTimeout = "TcpTimeout";

    /// uint8_t
    const char *_TcpServerTMC = "TcpServerTMC";
    const char *_TsiSp003Ver = "TsiSp003Ver";
    const char *_NumberOfSigns = "NumberOfSigns";
    const char *_NumberOfGroups = "NumberOfGroups";
    const char *_SlavePowerUpDelay = "SlavePowerUpDelay";
    const char *_ColourBits = "ColourBits";
    const char *_ColourLeds = "ColourLeds";
    const char *_IsResetLogAllowed = "IsResetLogAllowed";
    const char *_IsUpgradeAllowed = "IsUpgradeAllowed";
    const char *_TilesPerSlave = "TilesPerSlave";
    const char *_SlavesPerSign = "SlavesPerSign";
    const char *_DriverMode = "DriverMode";
    const char *_ColourRatio = "ColourRatio";
    const char *_Dimming = "Dimming";
    const char *_DimmingAdjTime = "DimmingAdjTime";
    const char *_LoadLastDisp = "LoadLastDisp";
    const char *_TestTMC = "TestTMC";
    const char *_TestSlave = "TestSlave";
    const char *_HasLanterns ="HasLanterns";

    // float
    const char *_LightSensorScale = "LightSensorScale";

    // items : 1-n
    const char *_IslusSpFrm = "IslusSpFrm"; // Islus specila frame

    // for SignX
    const char *_GroupId = "GroupId";
    const char *_IP = "IP";
    const char *_COM = "COM";
    const char *_RejectFrms = "RejectFrms"; // Reject frames

    Utils::Bits bIslusSpFrm{256};

    PRODUCT prodType;

    char mfcCode[11]{};

    Utils::Bits allowedAscii{128};

    Font *fonts[MAX_FONT + 1]{};

    std::vector<SignCfg> signCfg;

    uint16_t
        tcpTimeout,
        pixelRows,
        pixelColumns,
        coreOffsetX,
        coreOffsetY,
        slaveRqstInterval,
        slaveRqstStTo,
        slaveRqstExtTo,
        slaveSetStFrmDly,
        slaveDispDly,
        slaveCmdDly,
        driverFaultDebounce,
        mledFaultDebounce,
        sledFaultDebounce,
        overTempDebounce,
        selftestDebounce,
        offlineDebounce,
        lightSensorFaultDebounce,
        lanternFaultDebounce,
        slaveVoltageLow,
        slaveVoltageHigh,
        slaveVoltageDebounce,
        lightSensorScale;

    uint8_t
        tcpServerTMC,
        tsiSp003Ver,
        testTMC,
        testSlave,
        slavePowerUpDelay,
        colourBits, // 1,4,24
        isResetLogAllowed,
        isUpgradeAllowed,
        numberOfSigns,
        numberOfGroups,
        tilesPerSlave,
        slavesPerSign,
        powerOnDelay,
        dimmingAdjTime,
        driverMode,
        loadLastDisp,
        hasLanterns;

    uint8_t dimming[16];
    uint8_t colourRatio[4];
    uint16_t displayArea[4]; // "left,top,right,bottom"

    char colourLeds[5]{}; // with end '\0'

    uint8_t mappedColoursTable[10]{};
    uint8_t mappedColoursBitTable[10]{};

    Utils::Bits bFont{8};
    Utils::Bits bConspicuity{8};
    Utils::Bits bAnnulus{8};
    Utils::Bits bTxtFrmColour{32};
    Utils::Bits bGfxFrmColour{32};
    Utils::Bits bHrgFrmColour{32};

    // configurations calculated from other configurations
    uint32_t pixels;
    uint32_t pixelsPerSign;
    uint32_t pixelsPerSlave;

    SESR_SIGN_TYPE extStsRplSignType;
    SCR_SIGN_TYPE configRplSignType;
    int maxCoreLen;
    int gfx1CoreLen;
    int gfx4CoreLen;
    int gfx24CoreLen;

    int monitoringPort{-1};
    const char *_MonitoringPort = "MonitoringPort";
    int monitoringBps;
    const char *_MonitoringBps = "MonitoringBps";

    Utils::Bits bSimSlaves{16};
    const char *_SimSlaves = "SimSlaves";
};
