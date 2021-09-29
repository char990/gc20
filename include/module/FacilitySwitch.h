#pragma once


class FacilitySwitch
{
public:
    enum FS_STATE {OFF, AUTO,MSG1, MSG2};
    FacilitySwitch();
    ~FacilitySwitch();
    void PeriodicRun();

    bool IsChanged() { return isChanged; };
    void ClearChangeFlag() { isChanged = false; };

    FS_STATE Get() { return fsState; };

private:
    bool isChanged{false};
    FS_STATE fsState{FS_STATE::AUTO};
};

