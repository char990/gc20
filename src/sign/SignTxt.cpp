#include <cstring>
#include <uci/DbHelper.h>
#include <sign/SignTxt.h>

SignTxt::SignTxt(uint8_t signId)
:Sign(signId)
{

}

SignTxt::~SignTxt()
{

}

uint8_t *SignTxt::GetExtStatus(uint8_t *p)
{
    *p++=signId;
    *p++=0;         // text(0),gfx(1),advgfx(2)
    *p++=3;         // rows
    *p++=18;        // columns
    *p++=signErr;
    *p++=dimMode;
    *p++=dimLevel;
    *p++=0;
    return p;
}

