#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignHrg.h>


SignHrg::SignHrg(uint8_t signId)
:SignGfx(signId)
{

}

SignHrg::~SignHrg()
{

}

uint8_t *SignHrg::GetExtStatus(uint8_t *p)
{
    return SignGfx::GetExtStatus(p);
}
