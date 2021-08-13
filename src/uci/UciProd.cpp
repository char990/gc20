#include <string>
#include <uci/UciProd.h>

UciProd::UciProd()
{
    uciOpt.path = const_cast<char *>("/etc/config");
    uciOpt.package = const_cast<char *>("goblin");
}

void UciProd::LoadConfig()
{

    Dump();
}

void UciProd::Dump()
{


}

uint8_t UciProd::TsiSp003Ver()
{
    return 0x50;
}


char * UciProd::MfcCode()
{
    return  const_cast<char *>("GC20123456");
}
