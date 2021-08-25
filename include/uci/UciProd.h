#ifndef __PRODCFG_H__
#define __PRODCFG_H__

#include <tsisp003/TsiSp003Const.h>
#include <uci/UciCfg.h>
#include <module/Utils.h>

struct SignConnection
{
    uint32_t com_ip;
    int bps_port;
}

class UciProd : public UciCfg
{
public:
    UciProd();

    void LoadConfig() override;
    void Dump() override;

    int MaxTextFrmLen();
    int MinGfxFrmLen();
    int MaxGfxFrmLen();
    int MinHrgFrmLen();
    int MaxHrgFrmLen();

    int CharRows();
    int CharColumns();

    int PixelRows();
    int PixelColumns();
    int Pixels();

    // getter
    char * TsiSp003Ver() { return &tsiSp003Ver[0]};
    char * MfcCode() { return &mfcCode[0]};
    char * FontName(int i) { return &fontName[MAX_FONT+1][0]; };
    struct SignConnection * Sign(int i) { return &signs[i-1]; };

    uint16_t SlaveRqstInterval() { return slaveRqstInterval; };
    uint16_t SlaveRqstStTo() { return slaveRqstStTo; };
    uint16_t SlaveRqstExtTo() { return slaveRqstExtTo; };
    uint16_t SlaveSetStFrmDly() { return slaveSetStFrmDly; };
    uint16_t SlaveDispDly() { return slaveDispDly; };
    uint16_t SlaveCmdDly() { return slaveCmdDly; };

    uint16_t LightSensorMidday() { return LightSensorMidday; };
    uint16_t LightSensorMidnight() { return LightSensorMidnight; };
    uint16_t LightSensor18Hours() { return LightSensor18Hours; };
    uint16_t DriverFaultDebounce() { return DriverFaultDebounce; };
    uint16_t LedFaultDebounce() { return LedFaultDebounce; };
    uint16_t OverTempDebounce() { return OverTempDebounce; };
    uint16_t SelftestDebounce() { return SelftestDebounce; };
    uint16_t OfflineDebounce() { return OfflineDebounce; };
    uint16_t LightSensorFaultDebounce() { return LightSensorFaultDebounce; };
    uint16_t LanternFaultDebounce() { return LanternFaultDebounce; };
    uint16_t SlaveVoltageLow() { return SlaveVoltageLow; };
    uint16_t SlaveVoltageHigh() { return SlaveVoltageHigh; };
    uint16_t LightSensorScale() { return lightSensorScale; };

    uint8_t SlavePowerUpDelay() { return slavePowerUpDelay; };
    uint8_t ColourBits() { return colourBits; };
    uint8_t NumberOfSigns() { return numberOfSigns; };
    bool IsResetLogAllowed() { return isResetLogAllowed!=0; };
    bool IsUpgradeAllowed() { return isUpgradeAllowed!=0; };

    bool IsFont(int i) { return bFont.GetBit(i); };
    bool IsConspicuity(int i) { return bConspicuity.GetBit(i); };
    bool IsAnnulus(int i) { return bAnnulus.GetBit(i); };
    bool IsTxtFrmColour(int i) { return bTxtFrmColour.GetBit(i); };
    bool IsGfxFrmColour(int i) { return bGfxFrmColour.GetBit(i); };
    bool IsHrgFrmColour(int i) { return bHrgFrmColour.GetBit(i); };

private:
    const char * SECTION_NAME="ctrller_cfg";

    // string
    const char * _TsiSp003Ver="TsiSp003Ver";
    const char * _MfcCode="MfcCode";
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
    const char * _NumberOfSigns="NumberOfSigns";
    const char * _SlavePowerUpDelay="SlavePowerUpDelay";
    const char * _ColourBits="ColourBits";
    const char * _IsResetLogAllowed="IsResetLogAllowed";
    const char * _IsUpgradeAllowed="IsUpgradeAllowed";

    // float
    const char * _LightSensorScale="LightSensorScale";

    char tsiSp003Ver[8];
    char mfcCode[7];

    char fontName[MAX_FONT+1][8];
    
    struct SignConnection * signs;

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
        offlineDebounce
        lightSensorFaultDebounce,
        lanternFaultDebounce,
        slaveVoltageLow,
        slaveVoltageHigh,
        lightSensorScale;

    uint8_t
        slavePowerUpDelay,
        colourBits,
        isResetLogAllowed,
        isUpgradeAllowed,
        numberOfSigns;

    BitOption bFont;
    BitOption bConspicuity;
    BitOption bAnnulus;
    BitOption bTxtFrmColour;
    BitOption bGfxFrmColour;
    BitOption bHrgFrmColour;
};

#endif
