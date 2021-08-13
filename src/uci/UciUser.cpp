#include <uci/UciUser.h>

UciUser::UciUser()
{
    uciOpt.path = const_cast<char *>("/etc/config");
    uciOpt.package = const_cast<char *>("goblin_user");
}

void UciUser::LoadConfig()
{

    Dump();
}

void UciUser::Dump()
{


}

uint8_t UciUser::BroadcastId()
{
    return 0x00;
}
void UciUser::BroadcastId(uint8_t v)
{
}

uint8_t UciUser::DeviceId()
{
    return 0x01;
}
void UciUser::DeviceId(uint8_t v)
{
}

uint8_t UciUser::SeedOffset()
{
    return 0xB1;
}
void UciUser::SeedOffset(uint8_t v)
{
}

uint16_t UciUser::PasswdOffset()
{
    return 0x87A2;
}
void UciUser::PasswdOffset(uint16_t v)
{
}

uint16_t UciUser::SessionTimeout()
{
    return 120;
}
void UciUser::SessionTimeout(uint16_t v)
{
}

uint16_t UciUser::DisplayTimeout()
{
    return 21900;
}

void UciUser::DisplayTimeout(uint16_t v)
{
}
