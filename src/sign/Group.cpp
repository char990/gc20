#include <sign/Group.h>

Group::Group()
{

}

Group::~Group()
{

}

void Group::Add(Sign *sign)
{
    grpSigns.push_back(sign);
}
