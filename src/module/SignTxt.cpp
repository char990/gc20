#include <module/SignTxt.h>


uint8_t *SignTxt::GetExtStatus(uint8_t *p)
{
    *p++=signId;
    *p++=0;         // text(0),gfx(1),advgfx(2)
    *p++=3;         // rows
    *p++=18;        // columns
    *p++=signError;
    *p++=dimMode;
    *p++=dimLevel;
    *p++=0;
    return p;
}

