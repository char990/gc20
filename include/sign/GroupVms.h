#ifndef __GROUPVMS_H__
#define __GROUPVMS_H__

#include <sign/Group.h>
#include <module/ptcpp.h>
#include <module/BootTimer.h>

class GroupVms : public Group
{
public:
    GroupVms(uint8_t id);
    ~GroupVms();

    // hook for Group
    virtual void PeriodicHook() override;
    
private:
    uint8_t msgEnd;
    bool LoadDsNext();

    bool IsDsNextEmergency();

    int taskPlnLine;
    BootTimer task1Tmr;
    bool TaskPln(int * _ptLine);

    int taskMsgLine;
    BootTimer task2Tmr;
    bool TaskMsg(int * _ptLine);

};



#endif
