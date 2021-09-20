#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignAdg.h>


SignAdg::SignAdg(uint8_t signId)
:SignGfx(signId)
{

}

SignAdg::~SignAdg()
{

}

uint8_t *SignAdg::GetExtStatus(uint8_t *p)
{
    return SignGfx::GetExtStatus(p);
}
