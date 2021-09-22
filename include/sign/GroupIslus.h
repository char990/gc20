#ifndef __GROUPISLUS_H__
#define __GROUPISLUS_H__

#include <sign/Group.h>

class GroupIslus : public Group
{
public:
    GroupIslus(uint8_t id);
    ~GroupIslus();

    virtual void PeriodicHook() override;
private:
    /* data */
};

#endif
