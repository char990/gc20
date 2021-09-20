#ifndef __ISLUS_H__
#define __ISLUS_H__

#include <sign/UnitedSign.h>
#include <sign/Sign.h>
#include <sign/Slave.h>

class Islus : public UnitedSign
{
public:
    Islus(uint8_t sid);
    virtual ~Islus();
    virtual void Reset() override;
    //virtual void PeriodicRun() override;
    virtual uint8_t *GetStatus(uint8_t *p) override;
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
private:
    Sign *sign;
};

#endif
