#include "DbHelper.h"





void DbHelper::Init()
{

}

uint8_t DbHelper::TsiSp003Ver()
{
    return 0x50;
}


uint8_t DbHelper::BroadcastId()
{
    return 0x00;
}
void DbHelper::BroadcastId(uint8_t v)
{
}

uint8_t DbHelper::DeviceId()
{
    return 0x01;
}
void DbHelper::DeviceId(uint8_t v)
{
}

uint8_t DbHelper::SeedOffset()
{
    return 0xB1;
}
void DbHelper::SeedOffset(uint8_t v)
{
}

uint16_t DbHelper::PasswdOffset()
{
    return 0x87A2;
}
void DbHelper::PasswdOffset(uint16_t v)
{
}

uint16_t DbHelper::SessionTimeout()
{
    return 120;
}
void DbHelper::SessionTimeout(uint16_t v)
{
}

uint16_t DbHelper::DisplayTimeout()
{
    return 21900;
}

void DbHelper::DisplayTimeout(uint16_t v)
{
}

