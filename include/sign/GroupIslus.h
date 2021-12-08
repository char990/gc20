#pragma once

#include <sign/Group.h>

class GroupIslus : public Group
{
public:
    GroupIslus(uint8_t id);
    ~GroupIslus();

    virtual void PeriodicHook() override;

    virtual APP::ERROR DispAtomicFrm(uint8_t *id) override;

    bool TaskSetATF(int *_ptLine) override;

    virtual void IMakeFrameForSlave(uint8_t fid) override;

    virtual int ITransFrmWithOrBuf(uint8_t uciFrmId, uint8_t *dst) override;

private:
    uint8_t sATF;
    bool IsSpeedFrame(uint8_t frmId);
};
