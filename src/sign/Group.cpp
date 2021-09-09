#include <sign/Group.h>

Group::Group(uint8_t groupId)
:groupId(groupId)
{

}

Group::~Group()
{

}

void Group::Add(IUnitedSign *sign)
{
    grpSigns.push_back(sign);
}
