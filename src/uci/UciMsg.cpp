#include <uci/UciMsg.h>
#include <module/Utils.h>

UciMsg::UciMsg()
{
    uciOpt.path = const_cast<char *>("./config");
    uciOpt.package = const_cast<char *>("UciMsg");
	uciOpt.section = const_cast<char *>("msg");
}

void UciMsg::LoadConfig()
{

    Dump();
}

void UciMsg::Dump()
{

}

uint16_t UciMsg::ChkSum()
{
    return 0x00A0;
}
