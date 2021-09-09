#ifndef __IDISPUNIT_H__
#define __IDISPUNIT_H__

#include <cstdint>


class IUnitedSign
{
public:
    IUnitedSign(){};
    virtual ~IUnitedSign(){};
    virtual uint8_t * GetStatus(uint8_t *p)=0;
    virtual uint8_t * GetExtStatus(uint8_t *p)=0;
};

#endif
