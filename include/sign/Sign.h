#pragma once

#include <vector>
#include <sign/Slave.h>
#include <module/Debounce.h>
#include <sign/DeviceError.h>
#include <module/BootTimer.h>

/*
    Sign is fit to TSI-SP-003
    For VMS, it is a combination of slaves.
    For ISLUS, it is only one slave.
    It is used for SignStatusReply/SignExtStatusReply/SignConfigurationReply
    Sign is not for controlling slaves which are controlled in Group
*/

class Sign
{
public:
    Sign(uint8_t id);
    virtual ~Sign();

    void AddSlave(Slave * slave);

    uint8_t SignId() { return signId; };
    void Reset();

    uint8_t *GetStatus(uint8_t *p);
    virtual uint8_t *GetExtStatus(uint8_t *p) = 0;

    // set
    void SignErr(DEV::ERROR err, bool v) { signErr.Push(signId, err, v); };
    void SignErr(uint32_t v) { signErr.SetV(v); };
    void DimmingSet(uint8_t v) { dimmingSet = v; };
    void DimmingV(uint8_t v) { dimmingV = v; };
    void Device(uint8_t v) { device = v; };

    // get
    SignError & SignErr() { return signErr; };
    uint8_t SignTiles() { return vsSlaves.size()*Slave::numberOfTiles; };

    uint8_t * LedStatus(uint8_t * p);

    // set current display
    void SetReportDisp(uint8_t f, uint8_t m, uint8_t p)
    {
        reportPln = p;
        reportMsg = m;
        reportFrm = f;
    };

    void RefreshSlaveStatus();
    void RefreshSlaveExtSt();
    
    bool LightSnsrFailed(){return lightSnsrFailed;};

    Debounce dbcLightSnsr;
    Debounce dbcLux;
    Debounce dbcChain;
    Debounce dbcMultiLed;
    Debounce dbcSingleLed;
    Debounce dbcLantern;
    Debounce dbcOverTemp;
    Debounce dbcSelftest;
    Debounce dbcFan;
    Debounce dbcVoltage;


protected:
    uint8_t signId;
    std::vector<Slave *> vsSlaves;
    SignError signErr;

    uint8_t dimmingSet{0}, dimmingV{1};
    uint8_t DimmingMode() { return (dimmingSet == 0) ? 0 : 1; };
    uint8_t DimmingValue() { return (dimmingSet == 0) ? dimmingV : dimmingSet; };

    uint8_t device{1};

    uint8_t reportPln{0}, reportMsg{0}, reportFrm{0};

    uint8_t setFrm{0};
    uint8_t dispFrm{0};


    BootTimer luxTimer;     // set as 60 seconds, so luxDebounce counter would be 15
    int cnt18hours{0}, cntMidnignt{0}, cntMidday{0};
    bool lightSnsrFailed{false};
};
