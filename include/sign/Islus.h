#ifndef __ISLUS_H__
#define __ISLUS_H__

#include <sign/Sign.h>

class Islus : public Sign
{
public:
    Islus(uint8_t sid);
    ~Islus();
    virtual void PeriodicRun() override;
    virtual uint8_t *GetExtStatus(uint8_t *p) override;
};

#endif
