#ifndef __VMS_H__
#define __VMS_H__


#include <sign/Sign.h>

class Vms : public Sign
{
public:
    Vms(uint8_t sid);
    ~Vms();
    virtual void PeriodicRun() override;
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
};

#endif
