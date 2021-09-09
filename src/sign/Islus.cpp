#include <sign/Islus.h>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>
#include <sign/SignGfx.h>
#include <sign/SignHrg.h>

Islus::Islus(uint8_t sid)
{
    sign = new SignTxt(sid);
}

Islus::~Islus()
{
    delete sign;
}

uint8_t * Islus::GetStatus(uint8_t *p)
{
    return sign->GetStatus(p);
}

uint8_t * Islus::GetExtStatus(uint8_t *p)
{
    return sign->GetExtStatus(p);
}
