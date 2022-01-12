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
    uint32_t com_ip;
    int bps_port;
    std::string ToString();
    Utils::Bits rjctFrm{256};
};

class UciProd : public UciCfg
{
public:
    UciProd();
    ~UciProd();

    void LoadConfig() override;
    void Dump() override;

    // getter
    uint8_t TcpServerNTS() { return tcpServerNTS; };
    uint8_t TcpServerWEB() { return tcpServerWEB; };
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

    uint16_t LightSensorMidday() { return lightSensorMidday; };
    uint16_t LightSensorMidnight() { return lightSensorMidnight; };
    uint16_t LightSensor18Hours() { return lightSensor18Hours; };
    uint16_t DriverFaultDebounce() { return driverFaultDebounce; };
    uint16_t LedFaultDebounce() { return ledFaultDebounce; };
    uint16_t OverTempDebounce() { return overTempDebounce; };
    uint16_t SelftestDebounce() { return selftestDebounce; };
    uint16_t OfflineDebounce() { return offlineDebounce; };
    uint16_t LightSensorFaultDebounce() { return lightSensorFaultDebounce; };
    uint16_t LanternFaultDebounce() { return lanternFaultDebounce; };
    uint16_t SlaveVoltageLow() { return slaveVoltageLow; };
    uint16_t SlaveVoltageHigh() { return slaveVoltageHigh; };
    uint16_t SlaveVoltageDebounce() { return slaveVoltageDebounce; };
    uint16_t LightSensorScale() { return lightSensorScale; };

    uint8_t TsiSp003Ver() { return tsiSp003Ver; };
    PRODUCT ProdType() { return prodType; };
    uint8_t SlavePowerUpDelay() { return slavePowerUpDelay; };
    uint8_t ColourBits() { return colourBits; };
    uint8_t NumberOfSigns() { return numberOfSigns; };
    uint8_t NumberOfGroups() { return numberOfGroups; };
    uint8_t GetGroupIdOfSign(uint8_t signId) { return groupCfg[signId - 1]; };
    uint16_t PixelRows() { return pixelRows; };
    uint16_t PixelColumns() { return pixelColumns; };
    uint8_t TilesPerSlave() { return tilesPerSlave; };
    uint8_t SlavesPerSign() { return slavesPerSign; };

    uint8_t *Dimming() { return &dimming[0]; };
    uint8_t *ColourRatio() { return &colourRatio[0]; };
    uint8_t DriverMode() { return driverMode; };
    uint8_t DimmingAdjTime() { return dimmingAdjTime; };

    char *ColourLeds() { return colourLeds; };

    bool IsResetLogAllowed() { return isResetLogAllowed != 0; };
    bool IsUpgradeAllowed() { return isUpgradeAllowed != 0; };

    bool IsFont(int i) { return bFont.GetBit(i); };
    bool IsConspicuity(int i) { return bConspicuity.GetBit(i); };
    bool IsAnnulus(int i) { return bAnnulus.GetBit(i); };
    bool IsTxtFrmColourValid(int i) { return bTxtFrmColour.GetBit(i); };
    bool IsGfxFrmColourValid(int i) { return bGfxFrmColour.GetBit(i); };
    bool IsHrgFrmColourValid(int i) { return bHrgFrmColour.GetBit(i); };

    bool IsIslusSpFrm(int i) { return bIslusSpFrm.GetBit(i); };

    uint8_t MaxConspicuity() { return bConspicuity.GetMaxBit(); };
    uint8_t MaxFont() { return bConspicuity.GetMaxBit(); };
    uint8_t MaxAnnulus() { return bAnnulus.GetMaxBit(); };

    // configurations calculated from other configurations
    uint8_t CharRows(int fontX);
    uint8_t CharColumns(int fontX);
    uint32_t Pixels() { return pixels; };
    SESR_SIGN_TYPE ExtStsRplSignType() { return extStsRplSignType; };
    SCR_SIGN_TYPE ConfigRplSignType() { return configRplSignType; };
    int MaxFrmLen() { return maxFrmLen; };

    int MinTxtFrmLen() { return 1; };
    int MaxTxtFrmLen() { return 255; };
    int Gfx1FrmLen() { return gfx1FrmLen; };   // bytes
    int Gfx4FrmLen() { return gfx4FrmLen; };   // bytes
    int Gfx24FrmLen() { return gfx24FrmLen; }; // bytes

    //uint8_t *GroupCfg() { return groupCfg; };

private:
    ///  ---------- option -----------
    // string
    const char *_MfcCode = "MfcCode";
    const char *_ProdType = "ProdType";

    // int array
    const char *_Font = "Font";
    const char *_Conspicuity = "Conspicuity";
    const char *_Annulus = "Annulus";
    const char *_TxtFrmColour = "TxtFrmColour";
    const char *_GfxFrmColour = "GfxFrmColour";
    const char *_HrgFrmColour = "HrgFrmColour";
    const char *_GroupCfg = "GroupCfg";

    /// int
    const char *_SlaveRqstInterval = "SlaveRqstInterval";
    const char *_SlaveRqstStTo = "SlaveRqstStTo";
    const char *_SlaveRqstExtTo = "SlaveRqstExtTo";
    const char *_SlaveSetStFrmDly = "SlaveSetStFrmDly";
    const char *_SlaveDispDly = "SlaveDispDly";
    const char *_SlaveCmdDly = "SlaveCmdDly";

    /// uint16_t
    const char *_LightSensorMidday = "LightSensorMidday";
    const char *_LightSensorMidnight = "LightSensorMidnight";
    const char *_LightSensor18Hours = "LightSensor18Hours";
    const char *_DriverFaultDebounce = "DriverFaultDebounce";
    const char *_LedFaultDebounce = "LedFaultDebounce";
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

    /// uint8_t
    const char *_TcpServerNTS = "TcpServerNTS";
    const char *_TcpServerWEB = "TcpServerWEB";
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

    // float
    const char *_LightSensorScale = "LightSensorScale";

    // items : 1-n
    const char *_Sign = "Sign";               // Sign1-x
    const char *_RjctFrmSign = "RjctFrmSign"; // Reject frames of Sign1-x

    const char *_IslusSpFrm = "IslusSpFrm"; // Islus specila frame
    Utils::Bits bIslusSpFrm{256};

    PRODUCT prodType;

    char mfcCode[11];

    Font *fonts[MAX_FONT + 1];

    std::vector<SignCfg> signCfg;

    uint16_t
        pixelRows,
        pixelColumns,
        slaveRqstInterval,
        slaveRqstStTo,
        slaveRqstExtTo,
        slaveSetStFrmDly,
        slaveDispDly,
        slaveCmdDly,
        lightSensorMidday,
        lightSensorMidnight,
        lightSensor18Hours,
        driverFaultDebounce,
        ledFaultDebounce,
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
        tcpServerNTS,
        tcpServerWEB,
        tsiSp003Ver,
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
        driverMode;

    uint8_t dimming[16];
    uint8_t colourRatio[4];

    uint8_t *groupCfg;

    char colourLeds[5];

    uint8_t mappedColoursTable[10]{0};

    Utils::Bits bFont{8};
    Utils::Bits bConspicuity{8};
    Utils::Bits bAnnulus{8};
    Utils::Bits bTxtFrmColour{32};
    Utils::Bits bGfxFrmColour{32};
    Utils::Bits bHrgFrmColour{32};

    // configurations calculated from other configurations
    uint32_t pixels;
    SESR_SIGN_TYPE extStsRplSignType;
    SCR_SIGN_TYPE configRplSignType;
    int maxFrmLen;
    int gfx1FrmLen;
    int gfx4FrmLen;
    int gfx24FrmLen;
};
