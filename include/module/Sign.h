#ifndef __SIGN_H__
#define __SIGN_H__

#include <cstdint>
class Sign
{
public:
    void SetId(uint8_t sid, uint8_t gid);
    uint8_t SignId();
    uint8_t GroupId();
    uint8_t * GetStatus(uint8_t *p);
    virtual uint8_t * GetExtStatus(uint8_t *p)=0;

protected:
    uint8_t 
        groupId,
        signId,
        errCode,
        en_dis,
        frmId,
        frmRev,
        msgId,
        msgRev,
        plnId,
        plnRev;
    uint8_t
        signError,
        dimMode,
        dimLevel;
};

#endif
