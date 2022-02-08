#pragma once

#include <sign/Group.h>

class GroupIslus : public Group
{
public:
    GroupIslus(uint8_t id);
    ~GroupIslus();

    virtual void PeriodicHook() override;

    virtual APP::ERROR DispAtomicFrm(uint8_t *id) override;

    void TaskSetATFReset() override;
    // Must call TaskSetATFReset() before calling of TaskSetATF
    bool TaskSetATF(int *_ptLine) override;

    virtual void IMakeFrameForSlave(uint8_t fid) override;

    virtual int ITransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst) override;

private:
    uint8_t sATF;   // counter for SetFrame in TaskSetATF
    bool IsSpeedFrame(uint8_t frmId);
    std::vector<uint8_t> atfSt;     // atf setting status: 0:OK, others:NG
};
