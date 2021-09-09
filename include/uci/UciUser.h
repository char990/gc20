#ifndef __USERCFG_H__
#define __USERCFG_H__


#include <uci/UciCfg.h>
#include <uci/UciProd.h>

typedef struct ExtSw {
	uint16_t dispTime;
	uint8_t reserved;
	uint8_t emergency;
	uint8_t flashingOv;
	bool Equal(struct ExtSw* v)
	{
		return (	dispTime==v->dispTime &&
					reserved==v->reserved &&
					emergency==v->emergency &&
					flashingOv==v->flashingOv );
	};
}ExtSw;

class UciUser : public UciCfg
{
public:
    UciUser(UciProd &uciProd);
    ~UciUser();

    const char *DEFAULT_FILE; 

    void LoadConfig() override;
	void Dump() override;

    /// \brief  UserOpen() => Change option value => UserClose()
    void UserOpen();
    void UserClose();

    /// \brief  Load factory default. Call UserOpen/UserClose inside
    ///         Copy 'goblin_user.def' to 'goblin_user.def' and set DeviceId & BroadcastId
    void LoadFactoryDefault();

    /// --------getter--------
    uint8_t BroadcastId() { return broadcastId;};
    uint8_t DeviceId() { return deviceId;};
    uint8_t SeedOffset() { return seedOffset;};
    uint8_t Fan1OnTemp() { return fan1OnTemp;};
    uint8_t Fan2OnTemp() { return fan2OnTemp;};
    uint8_t OverTemp() { return overTemp;};
    uint8_t Humidity() { return humidity;};
    uint8_t DefaultFont() { return defaultFont;};
    uint8_t DefaultColour() { return defaultColour;};
    uint8_t LockedFrm() { return lockedFrm;};
    uint8_t LockedMsg() { return lockedMsg;};
    uint8_t LastFrmOn() { return lastFrmOn;};
    uint8_t ComPort() { return comPort;};
    uint8_t Tz() { return tz;};
    const char * TZ();
    uint16_t PasswordOffset() { return passwordOffset;};
    uint16_t SessionTimeout() { return sessionTimeout;};
    uint16_t DisplayTimeout() { return displayTimeout;};
    uint16_t SvcPort() { return svcPort;};
    uint16_t WebPort() { return webPort;};
    uint16_t MultiLedFaultThreshold() { return multiLedFaultThreshold;};
    int Baudrate() { return baudrate;};
    uint16_t *Luminance(){ return luminance; };
    uint8_t *DawnDusk(){ return dawnDusk; };
    uint8_t * GroupCfg() { return groupCfg;};
    uint8_t GetGroupIdOfSign(uint8_t signId) { return groupCfg[signId-1]; };
    ExtSw *ExtSwCfgX(int i){return &extSw[i];};


    /// --------setter--------
    void BroadcastId(uint8_t);
    void DeviceId(uint8_t);
    void SeedOffset(uint8_t);
    void Fan1OnTemp(uint8_t);
    void Fan2OnTemp(uint8_t);
    void OverTemp(uint8_t);
    void Humidity(uint8_t);
    void DefaultFont(uint8_t);
    void DefaultColour(uint8_t);
    void LockedFrm(uint8_t);
    void LockedMsg(uint8_t);
    void LastFrmOn(uint8_t);
    void ComPort(uint8_t);
    void Tz(uint8_t);
    void PasswordOffset(uint16_t);
    void SessionTimeout(uint16_t);
    void DisplayTimeout(uint16_t);
    void SvcPort(uint16_t);
    void WebPort(uint16_t);
    void MultiLedFaultThreshold(uint16_t);
    void Baudrate(int);
    void Luminance(uint16_t *);
    void DawnDusk(uint8_t *);
    void GroupCfg(uint8_t *);
    void ExtSwCfgX(int i, ExtSw *cfg);

private:

    UciProd &uciProd;
    
    uint8_t
        broadcastId,
        deviceId,
        seedOffset,
        fan1OnTemp,
        fan2OnTemp,
        overTemp,
        humidity,
        defaultFont,
        defaultColour,
        lockedFrm,
        lockedMsg,
        comPort,
        tz,
        lastFrmOn;

    uint16_t
        passwordOffset,
        sessionTimeout,
        displayTimeout,
        svcPort,
        webPort,
        multiLedFaultThreshold;

    int baudrate;

    char shakehandsPassword[11];
    uint16_t luminance[16];
    uint8_t dawnDusk[16];

    uint8_t * groupCfg;

    ExtSw extSw[3];

    const char * SECTION_NAME="user_cfg";

    const char * _DeviceId="DeviceId";
    const char * _BroadcastId="BroadcastId";
    const char * _SeedOffset="SeedOffset";
    const char * _PasswordOffset="PasswordOffset";
    const char * _SvcPort="SvcPort";
    const char * _WebPort="WebPort";
    const char * _Baudrate="Baudrate";

    const char * _OverTemp="OverTemp";
    const char * _Fan1OnTemp="Fan1OnTemp";
    const char * _Fan2OnTemp="Fan2OnTemp";
    const char * _Humidity="Humidity";

    const char * _SessionTimeout="SessionTimeout";
    const char * _DisplayTimeout="DisplayTimeout";
    const char * _DefaultFont="DefaultFont";
    const char * _DefaultColour="DefaultColour";

    const char * _MultiLedFaultThreshold="MultiLedFaultThreshold";

    const char * _LockedFrm="LockedFrm";
    const char * _LockedMsg="LockedMsg";
    const char * _LastFrmOn="LastFrmOn";
    

    const char * _TZ="TZ";
    const char * _ShakehandsPassword="ShakehandsPassword";
    const char * _Luminance="Luminance";
    const char * _DawnDusk="DawnDusk";
    const char * _ComPort="ComPort";
    const char * _GroupCfg="GroupCfg";
    const char * _ExtSw="ExtSw";

    void PrintExtSw(int i, char *buf);
    void PrintDawnDusk(char *buf);
    void PrintGroupCfg(char *buf);
    void PrintLuminance(char *buf);
    
};

#endif
