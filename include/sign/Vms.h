#ifndef __VMS_H__
#define __VMS_H__

#include <sign/IUnitedSign.h>
#include <sign/Sign.h>
#include <sign/Slave.h>

class Vms : public IUnitedSign
{
public:
    Vms(uint8_t sid);
    ~Vms();
    //virtual void PeriodicRun() override;
    virtual uint8_t *GetStatus(uint8_t *p) override;
    virtual uint8_t *GetExtStatus(uint8_t *p) override;

private:
    Sign *sign;

    Slave *slaves;
    uint8_t numberOfSlaves;
    uint8_t numberofTiles;
};

#endif
