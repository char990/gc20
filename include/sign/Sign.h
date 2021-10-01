#pragma once

#include <vector>
#include <sign/Slave.h>
#include <module/Debounce.h>

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
    void SignErr(uint8_t err);
    void DimmingSet(uint8_t v) { dimmingSet = v; };
    void DimmingV(uint8_t v) { dimmingV = v; };
    void Device(uint8_t v) { deviceSet = v; };

    // get
    uint8_t DimmingSet() { return dimmingV; };
    uint8_t DimmingV() { return dimmingV; };
    uint8_t Device() { return deviceV; };
    uint8_t SignTiles() { return vsSlaves.size()*Slave::numberOfTiles; };

    uint8_t * LedStatus(uint8_t * p);

    // set current display
    void CurrentDisp(uint8_t f, uint8_t m, uint8_t p)
    {
        reportPln = p;
        reportMsg = m;
        reportFrm = f;
    };

    void RefreshSlaveStatus();
    void RefreshSlaveExtSt();
    
    Debounce dbcLight;
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

    uint8_t signErr{0};

    uint8_t dimmingSet{0}, dimmingV{1};
    uint8_t DimmingMode() { return (dimmingSet == 0) ? 0 : 1; };
    uint8_t DimmingValue() { return (dimmingSet == 0) ? dimmingV : dimmingSet; };

    uint8_t deviceSet{1}, deviceV{1};

    uint8_t reportPln{0}, reportMsg{0}, reportFrm{0};

    uint8_t setFrm{0};
    uint8_t dispFrm{0};

};
