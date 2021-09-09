#ifndef __GROUP_H__
#define __GROUP_H__

#include <vector>
#include <sign/IUnitedSign.h>



class Group
{
public:
    Group(uint8_t groupId);
    ~Group();

    //Getter
    uint8_t GroupId() { return groupId; };

    // Add a sign into this group
    void Add(IUnitedSign * sign);

    std::vector<IUnitedSign *> grpSigns;

private:


    uint8_t
        groupId;
};

#endif
