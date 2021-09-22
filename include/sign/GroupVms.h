#ifndef __GROUPVMS_H__
#define __GROUPVMS_H__

#include <sign/Group.h>
#include <module/ptcpp.h>

class GroupVms : public Group, public Protothread
{
public:
    GroupVms(uint8_t id);
    ~GroupVms();

    // hook for Group
    virtual void PeriodicHook() override;
    
    // real task
    virtual bool Run() override;

private:
    bool LoadDsNext();
};



#endif
