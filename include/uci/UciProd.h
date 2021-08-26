#ifndef __PRODCFG_H__
#define __PRODCFG_H__

#include <tsisp003/TsiSp003Const.h>
#include <uci/UciCfg.h>
#include <module/Utils.h>
#include <string>

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
    char * MfcCode() { return &mfcCode[0]; };
    char * FontName(int i) { return &fontName[i][0]; };
    struct SignConnection * Sign(int i) { return &signs[i]; };

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
    struct uci_section *sec;

    ///  ---------- option -----------
    // string
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
    const char * _TsiSp003Ver="TsiSp003Ver";
    const char * _NumberOfSigns="NumberOfSigns";
    const char * _SlavePowerUpDelay="SlavePowerUpDelay";
    const char * _ColourBits="ColourBits";
    const char * _IsResetLogAllowed="IsResetLogAllowed";
    const char * _IsUpgradeAllowed="IsUpgradeAllowed";

    // float
    const char * _LightSensorScale="LightSensorScale";

    // items : 1-n
    const char * _Sign="Sign";      // Sign1-x
    // const char * _Font="Font";   // Font0-x


    /// ---------- option ----------
    char mfcCode[11];

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
        offlineDebounce,
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
        numberOfSigns,
        tsiSp003Ver;

    Utils::BitOption bFont;
    Utils::BitOption bConspicuity;
    Utils::BitOption bAnnulus;
    Utils::BitOption bTxtFrmColour;
    Utils::BitOption bGfxFrmColour;
    Utils::BitOption bHrgFrmColour;
    void ReadBitOption(struct uci_section *section, const char * option, Utils::BitOption &bo);
};

#endif
