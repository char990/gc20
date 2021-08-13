#include <module/Sign.h>

void Sign::SetId(uint8_t sid, uint8_t gid)
{
    signId = sid;
    groupId = gid;
}

uint8_t Sign::SignId()
{
    return signId;
}


uint8_t Sign::GroupId()
{
    return groupId;
}

uint8_t * Sign::GetStatus(uint8_t *p)
{
    *p++=signId;
    *p++=errCode;
    *p++=en_dis;
    *p++=frmId;
    *p++=frmRev;
    *p++=msgId;
    *p++=msgRev;
    *p++=plnId;
    *p++=plnRev;
    return p;
}
