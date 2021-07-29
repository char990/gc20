#ifndef __TSISP003APPVER10_H__
#define __TSISP003APPVER10_H__

#include "DbHelper.h"
#include "TsiSp003App.h"

/// \brief  This is the base of TsiSp003 Application layer
class TsiSp003AppVer10 : public TsiSp003App
{
public:
    TsiSp003AppVer10(bool & online);
    virtual ~TsiSp003AppVer10();

    virtual std::string Version() override { return "Ver1.0"; }

    virtual int Rx(uint8_t * data, int len) override;

private:
    void HeartbeatPoll(uint8_t * data, int len);
    void SignSetTextFrame(uint8_t * data, int len);
    void SignDisplayFrame(uint8_t * data, int len);
    void SignSetMessage(uint8_t * data, int len);
    void SignDisplayMessage(uint8_t * data, int len);
    void SignSetPlan(uint8_t * data, int len);
    void EnablePlan(uint8_t * data, int len);
    void DisablePlan(uint8_t * data, int len);
    void RequestEnabledPlans(uint8_t * data, int len);
    void SignRequestStoredFMP(uint8_t * data, int len);
};

#endif
