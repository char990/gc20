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

    bool TaskSetATF(int *_ptLine) override ; // no use for VMS, MyThrow("VMS can run ATF")

    virtual void IMakeFrameForSlave(uint8_t fid) override;

    virtual int ITransFrmWithOrBuf(uint8_t frmId, uint8_t *dst) override;

private:

};


