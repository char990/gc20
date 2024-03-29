#pragma once

#include <vector>

#include <uci/UciCfg.h>
#include <uci/UciHardware.h>
#include <module/Tz_AU.h>

class ExtInput
{
    public:
    uint16_t dispTime{0}; // 0:disabled
    uint8_t reserved;
    uint8_t emergency;  // 0:ON / 1:OFF
    uint8_t flashingOv; // 0:ON / 1:OFF
    bool Equal(struct ExtInput &v)
    {
        return (dispTime == v.dispTime &&
                reserved == v.reserved &&
                emergency == v.emergency &&
                flashingOv == v.flashingOv);
    };
};

class UciUserCfg : public UciCfg
{
public:
    ~UciUserCfg();

    const char *DEFAULT_FILE;

    void LoadConfig() override;
    void Dump() override;

    /// \brief  Load factory default. Call OpenSECTION/CommitCloseSectionForSave inside
    ///         Copy 'goblin_user.def' to 'goblin_user.def' and set DeviceId & BroadcastId
    void LoadFactoryDefault();

    /// --------getter--------
    uint8_t BroadcastId() { return broadcastId; };
    uint8_t DeviceId() { return deviceId; };
    uint8_t SeedOffset() { return seedOffset; };
    uint8_t Fan1OnTemp() { return fan1OnTemp; };
    uint8_t Fan2OnTemp() { return fan2OnTemp; };
    uint8_t OverTemp() { return overTemp; };
    uint8_t Humidity() { return humidity; };
    // uint8_t DefaultFont() { return defaultFont;};
    // uint8_t DefaultColour() { return defaultColour;};
    uint8_t LockedFrm() { return lockedFrm; };
    uint8_t LockedMsg() { return lockedMsg; };
    uint8_t LastFrmTime() { return lastFrmTime; };
    uint8_t CityId() { return cityId; };
    const char *City();
    Tz_AU *tz_AU{nullptr};
    uint16_t PasswordOffset() { return passwordOffset; };
    uint16_t SessionTimeoutSec() { return sessionTimeoutSec; }; // seconds
    uint16_t DisplayTimeoutMin() { return displayTimeoutMin; }; // minutes
    uint16_t TmcTcpPort() { return tmcTcpPort; };
    uint16_t WsPort() { return wsPort; };
    uint8_t TmcComPort() { return tmcComPort; };
    int TmcBaudrate() { return tmcBaudrate; };
    uint16_t MultiLedFaultThreshold() { return multiLedFaultThreshold; };
    uint16_t *Luminance() { return luminance; };
    uint8_t GetLuxLevel(int lux); // level:1-16
    uint8_t *DawnDusk() { return dawnDusk; };

    uint16_t LuxDayMin() { return luxDayMin; };
    uint16_t LuxNightMax() { return luxNightMax; };
    uint16_t Lux18HoursMin() { return lux18HoursMin; };
    uint8_t NightDimmingLevel() { return nightDimmingLevel; };
    uint8_t DayDimmingLevel() { return dayDimmingLevel; };
    uint8_t DawnDimmingLevel() { return dawnDimmingLevel; };

    const int EXT_SIZE = 4;
    ExtInput &ExtInputCfgX(int i)
    {
        return extInput.at(i);
    };
    const char *ShakehandsPassword() { return shakehandsPassword; };

    /// --------setter--------
    void BroadcastId(uint8_t);
    void DeviceId(uint8_t);
    void SeedOffset(uint8_t);
    void Fan1OnTemp(uint8_t);
    void Fan2OnTemp(uint8_t);
    void OverTemp(uint8_t);
    void Humidity(uint8_t);
    // void DefaultFont(uint8_t);
    // void DefaultColour(uint8_t);
    void LockedFrm(uint8_t);
    void LockedMsg(uint8_t);
    void LastFrmTime(uint8_t);
    void CityId(uint8_t);
    void PasswordOffset(uint16_t);
    void SessionTimeoutSec(uint16_t); // seconds
    void DisplayTimeoutMin(uint16_t); // minutes
    void TmcTcpPort(uint16_t);
    void WsPort(uint16_t);
    void TmcComPort(uint8_t);
    void TmcBaudrate(int);
    void MultiLedFaultThreshold(uint16_t);
    void Luminance(uint16_t *);
    void DawnDusk(uint8_t *);
    void ExtInputCfgX(int i, ExtInput &cfg);
    void ShakehandsPassword(const char *shake);
    void NightDimmingLevel(uint8_t);
    void DayDimmingLevel(uint8_t);
    void DawnDimmingLevel(uint8_t);
    void LuxDayMin(uint16_t);
    void LuxNightMax(uint16_t);
    void Lux18HoursMin(uint16_t);

private:
    uint8_t
        broadcastId,
        deviceId,
        seedOffset,
        fan1OnTemp,
        fan2OnTemp,
        overTemp,
        humidity,
        // defaultFont,
        // defaultColour,
        lockedFrm,
        lockedMsg,
        tmcComPort,
        cityId,
        lastFrmTime,
        nightDimmingLevel,
        dayDimmingLevel,
        dawnDimmingLevel;

    uint16_t
        luxDayMin,
        luxNightMax,
        lux18HoursMin,
        passwordOffset,
        sessionTimeoutSec,
        displayTimeoutMin,
        tmcTcpPort,
        wsPort,
        multiLedFaultThreshold;

    int tmcBaudrate;

    char shakehandsPassword[11]{};
    uint16_t luminance[16];
    uint8_t dawnDusk[16];

    std::vector<ExtInput> extInput{std::vector<ExtInput>(EXT_SIZE)};

    const char *_DeviceId = "DeviceId";
    const char *_BroadcastId = "BroadcastId";
    const char *_SeedOffset = "SeedOffset";
    const char *_PasswordOffset = "PasswordOffset";
    const char *_TmcTcpPort = "TmcTcpPort";
    const char *_WsPort = "WsPort";
    const char *_TmcComPort = "TmcComPort";
    const char *_TmcBaudrate = "TmcBaudrate";

    const char *_OverTemp = "OverTemp";
    const char *_Fan1OnTemp = "Fan1OnTemp";
    const char *_Fan2OnTemp = "Fan2OnTemp";
    const char *_Humidity = "Humidity";

    const char *_SessionTimeout = "SessionTimeout";
    const char *_DisplayTimeout = "DisplayTimeout";
    // const char * _DefaultFont="DefaultFont";
    // const char * _DefaultColour="DefaultColour";

    const char *_MultiLedFaultThreshold = "MultiLedFaultThreshold";

    const char *_LockedFrm = "LockedFrm";
    const char *_LockedMsg = "LockedMsg";
    const char *_LastFrmTime = "LastFrmTime";

    const char *_City = "City";
    const char *_ShakehandsPassword = "ShakehandsPassword";
    const char *_Luminance = "Luminance";
    const char *_DawnDusk = "DawnDusk";
    const char *_ExtInput = "ExtInput";

    const char *_LuxDayMin = "LuxDayMin";
    const char *_LuxNightMax = "LuxNightMax";
    const char *_Lux18HoursMin = "Lux18HoursMin";
    const char *_NightDimmingLevel = "NightDimmingLevel";
    const char *_DayDimmingLevel = "DayDimmingLevel";
    const char *_DawnDimmingLevel = "DawnDimmingLevel";

    void PrintExtSw(int i, char *buf);
    void PrintDawnDusk(char *buf);
    void PrintLuminance(char *buf);
};
