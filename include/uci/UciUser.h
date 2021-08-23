#ifndef __USERCFG_H__
#define __USERCFG_H__


#include <uci/UciCfg.h>
#include <uci/UciProd.h>

typedef struct ExtSwCfg {
	uint16_t dispTime;
	uint8_t reserved;
	uint8_t emergency;
	uint8_t flashingOv;
	bool Equal(struct ExtSwCfg* v)
	{
		return (	dispTime==v->dispTime &&
					reserved==v->reserved &&
					emergency==v->emergency &&
					flashingOv==v->flashingOv );
	};
}ExtSwCfg;

class UciUser : public UciCfg
{
public:
    UciUser(UciProd &uciProd);
    ~UciUser();

    bool IsChanged();

    void LoadConfig() override;
	void Dump() override;

    void LoadFactoryDefault();

    uint8_t BroadcastId() { return broadcastId;};
    void BroadcastId(uint8_t);

    uint8_t DeviceId() { return deviceId;};
    void DeviceId(uint8_t);

    uint8_t SeedOffset() { return seedOffset;};
    void SeedOffset(uint8_t);

    uint8_t Fan1OnTemp() { return fan1OnTemp;};
    void Fan1OnTemp(uint8_t);

    uint8_t Fan2OnTemp() { return fan2OnTemp;};
    void Fan2OnTemp(uint8_t);

    uint8_t OverTemp() { return overTemp;};
    void OverTemp(uint8_t);

    uint8_t Humidity() { return humidity;};
    void Humidity(uint8_t);

    uint8_t DefaultFont() { return defaultFont;};
    void DefaultFont(uint8_t);

    uint8_t DefaultColour() { return defaultColour;};
    void DefaultColour(uint8_t);

    uint8_t LockedFrm() { return lockedFrm;};
    void LockedFrm(uint8_t);

    uint8_t LockedMsg() { return lockedMsg;};
    void LockedMsg(uint8_t);

    uint8_t LastFrmTime() { return lastFrmTime;};
    void LastFrmTime(uint8_t);

    uint8_t ComportTMC() { return comportTMC;};
    void ComportTMC(uint8_t);

    uint8_t Tz() { return tz;};
    void Tz(uint8_t);
    const char * TZ();

    uint16_t PasswordOffset() { return passwordOffset;};
    void PasswordOffset(uint16_t);

    uint16_t SessionTimeout() { return sessionTimeout;};
    void SessionTimeout(uint16_t);
    
    uint16_t DisplayTimeout() { return displayTimeout;};
    void DisplayTimeout(uint16_t);

    uint16_t SvcPort() { return svcPort;};
    void SvcPort(uint16_t);

    uint16_t WebPort() { return webPort;};
    void WebPort(uint16_t);

    uint16_t MultiLedFaultThreshold() { return multiLedFaultThreshold;};
    void MultiLedFaultThreshold(uint16_t);

    int BaudrateTMC() { return baudrateTMC;};
    void BaudrateTMC(int);

    uint16_t *Luminance(){ return luminance; };
    void Luminance(uint16_t * p);

    uint8_t *DawnDusk(){ return dawnDusk; };
    void DawnDusk(uint8_t *p);

    int SignN() { return signN;};
    void SignN(int);

    uint8_t * GroupCfg() { return groupCfg;};

    ExtSwCfg *ExtSwCfgX(int i){return &extSwCfg[i];};

    void SetUciUserConfig(const char *option, char * value);

private:

    bool isChanged;
    UciProd &uciProd;
    struct uci_section *sec;
    
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
        comportTMC,
        tz,
        lastFrmTime;

    uint16_t
        passwordOffset,
        sessionTimeout,
        displayTimeout,
        svcPort,
        webPort,
        multiLedFaultThreshold;

    int baudrateTMC;

    char shakehandsPassword[11];
    uint16_t luminance[16];
    uint8_t dawnDusk[16];

    int signN;
    uint8_t * groupCfg;

    ExtSwCfg extSwCfg[3];

    const char * SECTION_NAME="user_cfg";

    const char * _DeviceId="DeviceId";
    const char * _BroadcastId="BroadcastId";
    const char * _SeedOffset="SeedOffset";
    const char * _PasswordOffset="PasswordOffset";
    const char * _SvcPort="SvcPort";
    const char * _WebPort="WebPort";
    const char * _BaudrateTMC="BaudrateTMC";

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
    const char * _LastFrmTime="LastFrmTime";
    

    const char * _TZ="TZ";
    const char * _ShakehandsPassword="ShakehandsPassword";
    const char * _Luminance="Luminance";
    const char * _DawnDusk="DawnDusk";
    const char * _ComportTMC="ComportTMC";
    const char * _GroupCfg="GroupCfg";
    const char * _ExtSw_Cfg="ExtSw_Cfg";

    void PrintExtSwCfg(int i, char *buf);
    void PrintDawnDusk(char *buf);
    void PrintGroupCfg(char *buf);
    void PrintLuminance(char *buf);
    
};

#endif
