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

    virtual void MakeFrameForSlave(uint8_t fid) override;

    virtual void TransFrmToOrBuf(uint8_t fid) override;

private:
    uint8_t sATF;
};
