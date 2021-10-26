#pragma once

#include <vector>
#include <sign/Slave.h>
#include <module/Debounce.h>
#include <sign/DeviceError.h>
#include <module/BootTimer.h>
#include <tsisp003/TsiSp003Const.h>
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

    void AddSlave(Slave *slave);

    uint8_t SignId() { return signId; };

    void ClearFaults();

    uint8_t *GetStatus(uint8_t *p);
    virtual uint8_t *GetExtStatus(uint8_t *p) = 0;

    // set
    void SignErr(DEV::ERROR err, bool v) { signErr.Push(signId, err, v); };
    void SignErr(uint32_t v) { signErr.SetV(v); };
    void DimmingSet(uint8_t v) { dimmingSet = v; };
    void DimmingV(uint8_t v) { dimmingV = v; };
    void Device(uint8_t v) { device = v; };

    // get
    SignError &SignErr() { return signErr; };
    uint8_t SignTiles() { return vsSlaves.size() * Slave::numberOfTiles; };

    uint8_t *LedStatus(uint8_t *p);

    // set current display
    void SetReportDisp(uint8_t f, uint8_t m, uint8_t p)
    {
        reportPln = p;
        reportMsg = m;
        reportFrm = f;
    };

    void RefreshSlaveStatusAtExtSt();

    // Init Debounce-Fault/Fault-flag from signErr
    void InitFaults();

    // Fatal Error
    Debounce chainFault;
    Debounce multiLedFault;
    Debounce selftestFault;
    Debounce voltageFault;
    Utils::State5 overTempFault;

    Utils::State5 fatalError;   // combine chain, multiLed, selftest, voltage and overtemp

    // Normal Error
    Utils::State5 luminanceFault;
    Debounce lanternFault;
    Debounce singleLedFault;
    
    int8_t CurTemp() { return curTemp; };
    int8_t MaxTemp() { return maxTemp; };
    uint16_t Voltage() { return voltage; };
    uint16_t Lux() { return lux; };

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

    uint8_t tflag{255};
    uint8_t lasthour{255};
    Debounce lsConnectionFault;
    Debounce ls18hoursFault;
    Debounce lsMidnightFault;
    Debounce lsMiddayFault;

    int8_t curTemp{0}, maxTemp{0};
    
    uint16_t voltage{0}, lux{0};

    void DbncFault(Debounce & dbc, DEV::ERROR err, const char * info=nullptr);
};
