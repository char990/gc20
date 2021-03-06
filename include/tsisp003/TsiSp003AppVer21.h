#pragma once

#include <tsisp003/TsiSp003App.h>

/// \brief  This is the base of TsiSp003 Application layer
class TsiSp003AppVer21 : public TsiSp003App
{
public:
    TsiSp003AppVer21();
    virtual ~TsiSp003AppVer21();

    virtual std::string Version() override { return "Ver2.1"; }

    virtual int Rx(uint8_t * data, int len) override;

protected:
    uint8_t * Time2Buf(uint8_t *p);
    void SignSetFrame(uint8_t * data, int len);

private:
    void UpdateTime(uint8_t *data, int len);
    void HeartbeatPoll(uint8_t * data, int len);
    void SignStatusReply();
    void SystemReset(uint8_t * data, int len);
    void SignDisplayFrame(uint8_t * data, int len);
    void SignSetMessage(uint8_t * data, int len);
    void SignDisplayMessage(uint8_t * data, int len);
    void SignSetPlan(uint8_t * data, int len);
    void EnDisPlan(uint8_t * data, int len);
    void RequestEnabledPlans(uint8_t * data, int len);
    void SignSetDimmingLevel(uint8_t *data, int len);
    void PowerOnOff(uint8_t * data, int len);
    void DisableEnableDevice(uint8_t * data, int len);
    void SignRequestStoredFMP(uint8_t * data, int len);
    void SignExtendedStatusRequest(uint8_t * data, int len);
    void RetrieveFaultLog(uint8_t * data, int len);
    void ResetFaultLog(uint8_t * data, int len);
};

