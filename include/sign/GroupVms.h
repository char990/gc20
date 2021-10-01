#pragma once


#include <sign/Group.h>
#include <module/BootTimer.h>

class GroupVms : public Group
{
public:
    GroupVms(uint8_t id);
    ~GroupVms();

    // -------------- hook for Group ---------------
    // called in PeriodicRun
    void PeriodicHook() override;
    
    APP::ERROR DispAtomicFrm(uint8_t *id) override;

private:

};


