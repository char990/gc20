#ifndef __SIGN_H__
#define __SIGN_H__

#include <cstdint>
#include <module/IPeriodicRun.h>
#include <tsisp003/TsiSp003Const.h>

struct Display
{
    Display():dispState(CTRLLER_STATE::DISPSTATE::DISP_NONE){};
    enum CTRLLER_STATE::DISPSTATE dispState;
    uint8_t plnId;
    uint8_t msgId;
    uint8_t frmId;
};

class Sign// : public IPeriodicRun
{
public:
    Sign(uint8_t sid);
    virtual ~Sign(){};

    //void PeriodicRun() override = 0;
    
    uint8_t * GetStatus(uint8_t *p);
    virtual uint8_t * GetExtStatus(uint8_t *p)=0;

    void SetDimming(uint8_t dimming);

protected:
    uint8_t
        signId,
        signErr;
    
    uint8_t       // for sign status reply
        en_dis,
        reportFrmId,
        reportMsgId,
        reportPlnId;

    uint8_t       // for sign ext-status reply
        dimMode,
        dimLevel;

    // settings
    uint8_t
        dimming;


    Display currentDisp;
    Display currentDispBak;
    Display nextDisp;
};


#endif
