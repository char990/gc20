#include <uci/DbHelper.h>


DbHelper::~DbHelper()
{
}

void DbHelper::Init()
{
    uciProd.LoadConfig();
    uciUser.LoadConfig();
    uciFrm.LoadConfig();
}

uint16_t DbHelper::HdrChksum()
{
    return uciFrm.ChkSum() + uciMsg.ChkSum() + uciPln.ChkSum();
}

