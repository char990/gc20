#ifndef __UCIGROUPPLAN_H__
#define __UCIGROUPPLAN_H__

#include <string>
#include <uci/UciCfg.h>
#include <uci/GrpPln.h>

class UciGroupPlan : public UciCfg
{
public:
    UciGroupPlan();
    ~UciGroupPlan();

    void LoadConfig() override;

	void Dump() override;

    bool IsPlanEnabled(uint8_t gid, uint8_t pid);
    void EnablePlan(uint8_t gid, uint8_t pid);
    void DisablePlan(uint8_t gid, uint8_t pid);


private:
    GrpPln * grpPln;
    uint8_t grpCnt;
    int PrintGrpPln(uint8_t gid, char *buf);
    void SaveGrpPln(uint8_t gid);
};

#endif
