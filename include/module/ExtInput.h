#pragma once


class ExtInput
{
public:
    enum EXT_STATE {NONE, M3, M4, M5};
    ExtInput();
    ~ExtInput();
    void PeriodicRun();

    bool IsChanged() { return isChanged; };
    void ClearChangeFlag() { isChanged = false; };

    void Reset();
    
    EXT_STATE Get() { return state; };

private:
    bool isChanged{false};
    EXT_STATE state{EXT_STATE::NONE};
};


