#ifndef __FACILITYSWITCH_H__
#define __FACILITYSWITCH_H__

class FacilitySwitch
{
public:
    enum FS_STATE {OFF, AUTO,MSG1, MSG2};
    void PeriodicRun();

    bool IsChanged() { return isChanged; };
    void ClearChangeFlag() { isChanged = false; };

    FS_STATE Get() { return fsState; };

private:
    bool isChanged;
    FS_STATE fsState;
};

#endif

