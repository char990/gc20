#ifndef __GRPPLN_H__
#define __GRPPLN_H__

#include <cstdint>

class GrpPln
{
public:
    GrpPln();
    bool IsPlanEnabled(uint8_t id);
    void EnablePlan(uint8_t id);
    void DisablePlan(uint8_t id);
private:
    uint8_t enabledPln[255];
};

#endif
