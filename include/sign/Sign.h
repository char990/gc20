#ifndef __SIGN_H__
#define __SIGN_H__

#include <cstdint>
#include <module/IPeriodicRun.h>
#include <tsisp003/TsiSp003Const.h>


#define SIGNTYPE_SIZE 2
extern const char *SIGNTYPE[SIGNTYPE_SIZE];

struct Display
{
    Display():dispState(CTRLLER_STATE::DISPSTATE::DISP_NONE){};
    enum CTRLLER_STATE::DISPSTATE dispState;
    uint8_t plnId;
    uint8_t msgId;
    uint8_t frmId;
}

class Sign : public IPeriodicRun
{
public:
    Sign();
    virtual ~Sign();

    void PeriodicRun() override = 0;
    
    void SetId(uint8_t sid, uint8_t gid);
    uint8_t SignId();
    uint8_t GroupId();
    uint8_t * GetStatus(uint8_t *p);
    virtual uint8_t * GetExtStatus(uint8_t *p)=0;

protected:
    uint8_t
        groupId;

    uint8_t
        signId,
        errCode,
        en_dis,
        reportFrmId,
        reportMsgId,
        reportPlnId;

    uint8_t
        dimMode,
        dimLevel;

    Display currentDisp;
    Display currentDispBak;
    Display nextDisp;
};


#endif
