#pragma once


#include <sign/Group.h>

class GroupIslus : public Group
{
public:
    GroupIslus(uint8_t id);
    ~GroupIslus();

    virtual void Add(Sign * sign) override;


    virtual void PeriodicHook() override;
private:
    /* data */
};

