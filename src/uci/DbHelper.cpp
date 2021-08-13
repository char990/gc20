#include <uci/DbHelper.h>





void DbHelper::Init()
{
    uciFrm.LoadConfig();
}


uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}
