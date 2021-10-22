#pragma once

#include <cstdint>
#include <cstdio>
#include <module/Utils.h>
//#include <module/MyDbg.h>

/// \brief bool debounce
/// start at invalid
/// if true over true_cnt, valid, value=true
/// if false over false_cnt, valid, value=false
class Debounce
{
public:
    Debounce(){};
    Debounce(int cnt)
    {
        SetCNT(cnt);
        Reset();
    }

    Debounce(int true_cnt, int false_cnt)
    {
        SetCNT(true_cnt, false_cnt);
        Reset();
    }

    /// \breif Reset debounce state
    /// clear counter, invalid state, keep value
    void Reset()
    {
        cnt0 = 0;
        cnt1 = 0;
        changed = false;
        value = Utils::STATE3::S_NA;
    }

    /// \breif Reset debounce state
    /// true    : valid & changed, value = true
    /// false   : valid & changed, value = false
    void SetState(bool v)
    {
        if (v)
        {
            cnt0 = 0;
            cnt1 = CNT1;
            changed = true;
            value = Utils::STATE3::S_1;
        }
        else
        {
            cnt0 = CNT0;
            cnt1 = 0;
            changed = true;
            value = Utils::STATE3::S_0;
        }
    }

    /// \brief  Set counter, should follow Reset/SetState
    void SetCNT(int true_cnt, int false_cnt)
    {
        CNT1 = true_cnt;
        CNT0 = false_cnt;
    }

    void SetCNT(int cnt)
    {
        CNT1 = cnt;
        CNT0 = cnt;
    }

    // Called regularly
    void Check(bool v)
    {
        if (v)
        {
            if (cnt1 < CNT1)
            {
                cnt1++;
            }
            if (cnt1 >= CNT1)
            {
                cnt0 = 0;
                cnt1 = CNT1;
                if (value != Utils::STATE3::S_1)
                {
                    changed = true;
                    value = Utils::STATE3::S_1;
                    //PrintDbg("changed v=1\n");
                }
            }
        }
        else
        {
            if (cnt0 < CNT0)
            {
                cnt0++;
            }
            if (cnt0 >= CNT0)
            {
                cnt1 = 0;
                cnt0 = CNT0;
                if (value != Utils::STATE3::S_0)
                {
                    changed = true;
                    value = Utils::STATE3::S_0;
                    //PrintDbg("changed v=0\n");
                }
            }
        }
    }

    void ResetCnt()
    {
        cnt0 = 0;
        cnt1 = 0;
    }

    bool changed = false;
    bool IsValid() { return value != Utils::STATE3::S_NA; };

    Utils::STATE3 Value(void)
    {
        return value;
    };

private:
    Utils::STATE3 value{Utils::STATE3::S_NA};
    int CNT1{1};
    int CNT0{1};
    int cnt1{0};
    int cnt0{0};
};
