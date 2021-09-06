#ifndef __GROUP_H__
#define __GROUP_H__

#include <vector>
#include <sign/Sign.h>



class Group
{
public:
    Group();
    ~Group();

    //Getter
    uint8_t GroupId() { return groupId; };
    uint8_t DimMode() { return dimMode; };
    uint8_t DimLevel() { return dimLevel; };

    // Add a sign into this group
    void Add(Sign * sign);
    vector<Sign *> &GrpSigns(){ return grpSigns; };

private:

    vector<Sign *> grpSigns;

    uint8_t
        groupId,
        dimMode,
        dimLevel;
};

#endif
