#pragma once


#include <sign/Group.h>

class GroupIslus : public Group
{
public:
    GroupIslus(uint8_t id);
    ~GroupIslus();

    virtual void PeriodicHook() override;

    virtual APP::ERROR DispAtomicFrm(uint8_t *id) override;
private:
    /* data */
};
