#ifndef __ISLUS_H__
#define __ISLUS_H__

#include <sign/IUnitedSign.h>
#include <sign/Sign.h>
#include <sign/Slave.h>

class Islus : public IUnitedSign
{
public:
    Islus(uint8_t sid);
    ~Islus();
    //virtual void PeriodicRun() override;
    virtual uint8_t *GetStatus(uint8_t *p) override;
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
private:
    Sign *sign;
};

#endif
