#ifndef __PRODCFG_H__
#define __PRODCFG_H__

#include <tsisp003/TsiSp003Const.h>
#include <uci/UciCfg.h>
#include <module/Utils.h>
#include <string>
#include <uci/Font.h>

struct SignConnection
{
    uint32_t com_ip;
    int bps_port;
    std::string ToString();
};

class UciProd : public UciCfg
{
public:
    UciProd();
    ~UciProd();

    void LoadConfig() override;
    void Dump() override;


    // getter
    char * MfcCode() { return &mfcCode[0]; };
    Font * GetFont(int i) { return &fonts[i]; };
    struct SignConnection * SignCn(int i) { return &signCn[i]; };
    uint8_t * MappedColoursTable() {return &mappedColoursTable[0]; };

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
    uint16_t LightSensorScale() { return lightSensorScale; };

    uint8_t TsiSp003Ver() { return tsiSp003Ver;};
    uint8_t SignType() { return signType;};
    uint8_t SlavePowerUpDelay() { return slavePowerUpDelay; };
    uint8_t ColourBits() { return colourBits; };
    uint8_t NumberOfSigns() { return numberOfSigns; };
    uint8_t PixelRowsPerTile() { return pixelRowsPerTile; };
    uint8_t PixelColumnsPerTile() { return pixelColumnsPerTile; };
    uint8_t TileRowsPerSlave() { return tileRowsPerSlave; };
    uint8_t TileColumnsPerSlave() { return tileColumnsPerSlave; };
    uint8_t SlaveRowsPerSign() { return slaveRowsPerSign; };
    uint8_t SlaveColumnsPerSign() { return slaveColumnsPerSign; };

    bool IsResetLogAllowed() { return isResetLogAllowed!=0; };
    bool IsUpgradeAllowed() { return isUpgradeAllowed!=0; };

    bool IsFont(int i) { return bFont.GetBit(i); };
    bool IsConspicuity(int i) { return bConspicuity.GetBit(i); };
    bool IsAnnulus(int i) { return bAnnulus.GetBit(i); };
    bool IsTxtFrmColour(int i) { return bTxtFrmColour.GetBit(i); };
    bool IsGfxFrmColour(int i) { return bGfxFrmColour.GetBit(i); };
    bool IsHrgFrmColour(int i) { return bHrgFrmColour.GetBit(i); };

    // configurations calculated from other configurations
    uint8_t CharRows(int fontX);
    uint8_t CharColumns(int fontX);
    uint16_t PixelRows() {return pixelRows;};
    uint16_t PixelColumns() {return pixelColumns;};
    uint32_t Pixels() {return pixels;};
    uint8_t ExtStsRplSignType() { return extStsRplSignType; };
    uint8_t ConfigRplSignType() { return configRplSignType; };
    int MinGfxFrmLen() { return minGfxFrmLen; };
    int MaxGfxFrmLen() { return maxGfxFrmLen; };

private:
    const char * SECTION_NAME="ctrller_cfg";
    ///  ---------- option -----------
    // string
    const char * _MfcCode="MfcCode";
    const char * _SignType="SignType";
    // int array
    const char * _Font="Font";
    const char * _Conspicuity="Conspicuity";
    const char * _Annulus="Annulus";
    const char * _TxtFrmColour="TxtFrmColour";
    const char * _GfxFrmColour="GfxFrmColour";
    const char * _HrgFrmColour="HrgFrmColour";
    /// int
    const char * _SlaveRqstInterval="SlaveRqstInterval";
    const char * _SlaveRqstStTo="SlaveRqstStTo";
    const char * _SlaveRqstExtTo="SlaveRqstExtTo";
    const char * _SlaveSetStFrmDly="SlaveSetStFrmDly";
    const char * _SlaveDispDly="SlaveDispDly";
    const char * _SlaveCmdDly="SlaveCmdDly";
    /// uint16_t
    const char * _LightSensorMidday="LightSensorMidday";
    const char * _LightSensorMidnight="LightSensorMidnight";
    const char * _LightSensor18Hours="LightSensor18Hours";
    const char * _DriverFaultDebounce="DriverFaultDebounce";
    const char * _LedFaultDebounce="LedFaultDebounce";
    const char * _OverTempDebounce="OverTempDebounce";
    const char * _SelftestDebounce="SelftestDebounce";
    const char * _OfflineDebounce="OfflineDebounce";
    const char * _LightSensorFaultDebounce="LightSensorFaultDebounce";
    const char * _LanternFaultDebounce="LanternFaultDebounce";
    const char * _SlaveVoltageLow="SlaveVoltageLow";
    const char * _SlaveVoltageHigh="SlaveVoltageHigh";
    /// uint8_t
    const char * _TsiSp003Ver="TsiSp003Ver";
    const char * _NumberOfSigns="NumberOfSigns";
    const char * _SlavePowerUpDelay="SlavePowerUpDelay";
    const char * _ColourBits="ColourBits";
    const char * _IsResetLogAllowed="IsResetLogAllowed";
    const char * _IsUpgradeAllowed="IsUpgradeAllowed";
    const char * _PixelRowsPerTile="PixelRowsPerTile";
    const char * _PixelColumnsPerTile="PixelColumnsPerTile";
    const char * _TileRowsPerSlave="TileRowsPerSlave";
    const char * _TileColumnsPerSlave="TileColumnsPerSlave";
    const char * _SlaveRowsPerSign="SlaveRowsPerSign";
    const char * _SlaveColumnsPerSign="SlaveColumnsPerSign";

    // float
    const char * _LightSensorScale="LightSensorScale";

    // items : 1-n
    const char * _Sign="Sign";      // Sign1-x
    // const char * _Font="Font";   // Font0-x

    char mfcCode[11];

    Font *fonts[MAX_FONT+1];

    struct SignConnection * signCn;

    uint16_t
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
        lightSensorScale;

    uint8_t
        tsiSp003Ver,
        slavePowerUpDelay,
        colourBits,
        isResetLogAllowed,
        isUpgradeAllowed,
        numberOfSigns,
        signType,
        pixelRowsPerTile,
        pixelColumnsPerTile,
        tileRowsPerSlave,
        tileColumnsPerSlave,
        slaveRowsPerSign,
        slaveColumnsPerSign;

	uint8_t mappedColoursTable[10];				// colourtable[0] is always 0

    Utils::BitOption bFont;
    Utils::BitOption bConspicuity;
    Utils::BitOption bAnnulus;
    Utils::BitOption bTxtFrmColour;
    Utils::BitOption bGfxFrmColour;
    Utils::BitOption bHrgFrmColour;

    // configurations calculated from other configurations
    uint16_t pixelRows;
    uint16_t pixelColumns;
    uint32_t pixels;
    uint8_t extStsRplSignType;
    uint8_t configRplSignType;
    
    int minGfxFrmLen;
    int maxGfxFrmLen;
};

#endif
