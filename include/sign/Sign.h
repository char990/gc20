#ifndef __SIGN_H__
#define __SIGN_H__

#include <cstdint>
#include <sign/SignCfg.h>
#include <module/IPeriodicRun.h>

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
    static SignCfg signCfg;

protected:
    uint8_t
        signId,
        errCode,
        en_dis,
        reportFrmId,
        reportMsgId,
        reportPlnId,
        groupId,
        signError,
        dimMode,
        dimLevel;
};

#endif
