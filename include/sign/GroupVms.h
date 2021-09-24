#pragma once


#include <sign/Group.h>
#include <module/BootTimer.h>

class GroupVms : public Group
{
public:
    GroupVms(uint8_t id);
    ~GroupVms();

    virtual void Add(Sign * sign) override;

    // -------------- hook for Group ---------------
    // called in PeriodicRun
    virtual void PeriodicHook() override;
    
    
private:


};

