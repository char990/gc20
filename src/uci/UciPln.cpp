#include <uci/UciPln.h>
#include <module/Utils.h>

UciPln::UciPln()
{
    uciOpt.path = const_cast<char *>("./config");
    uciOpt.package = const_cast<char *>("UciPln");
	uciOpt.section = const_cast<char *>("pln");
}

void UciPln::LoadConfig()
{

    Dump();
}

void UciPln::Dump()
{


}

uint16_t UciPln::ChkSum()
{
    return 0x000A;
}