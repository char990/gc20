#ifndef __USERCFG_H__
#define __USERCFG_H__


#include <uci/UciCfg.h>

struct GrpSign
{
    uint8_t sId;
    uint8_t gId;
};

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
    UciUser();
    ~UciUser();

    bool IsChanged();

    void LoadConfig() override;
	void Dump() override;

    void LoadFactoryDefault();

    uint8_t BroadcastAddr() { return broadcastAddr;};
    void BroadcastAddr(uint8_t);

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





    uint16_t PasswdOffset() { return passwdOffset;};
    void PasswdOffset(uint16_t);

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

    int SignN() { return signN;};
    void SignN(int);

    struct GrpSign * GrpSign() { return grpSign;};

    ExtSwCfg *ExtSwCfgX(int i);

    void Commit();

private:
    bool isChanged;
    struct uci_section *sec;
    
    uint8_t
        broadcastAddr,
        deviceId,
        seedOffset,
        fan1OnTemp,
        fan2OnTemp,
        overTemp,
        humidity,
        defaultFont,
        defaultColour,
        lockedFrm,
        lockedMsg;

    uint16_t
        passwdOffset,
        sessionTimeout,
        displayTimeout,
        svcPort,
        webPort,
        multiLedFaultThreshold;

    int baudrateTMC;

    char tz[16];
    char shakehandsPassword[11];
    uint16_t luminance[16];
    uint8_t dawnDusk[16];

    int signN;
    struct GrpSign * grpSign;

    ExtSwCfg extSwCfg[3];
};

#endif
