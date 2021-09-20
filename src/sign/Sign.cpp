#include <sign/Sign.h>
#include <uci/DbHelper.h>

Sign::Sign(uint8_t sid)
    : signId(sid), signErr(0), en_dis(1),
      reportFrmId(0), reportMsgId(0), reportPlnId(0)
{
}


void Sign::SetDimming(uint8_t dimming_)
{
    if(dimming_<16)
    {
        dimming = dimming_;
    }
}
