#pragma once

#include <cstdint>
#include <cstdio>
#include <module/Utils.h>
//#include <module/MyDbg.h>

/// \brief bool debounce
/// start at invalid
/// if true over true_cnt, valid, value=true
/// if false over false_cnt, valid, value=false
class Debounce : public Utils::State5
{
public:
    Debounce(){};
    Debounce(int cnt)
    {
        SetCNT(cnt);
    }

    Debounce(int true_cnt, int false_cnt)
    {
        SetCNT(true_cnt, false_cnt);
    }

    /// \breif Reset debounce state
    /// clear counter, invalid state, keep value
    void Reset()
    {
        cnt0 = 0;
        cnt1 = 0;
        v = Utils::STATE5::S5_NA;
    }

    /// \breif Reset debounce state
    /// true    : valid=true, no edge, value = true
    /// false   : valid=true, no edge, value = false
    void SetState(bool s)
    {
        if (s)
        {
            cnt0 = 0;
            cnt1 = CNT1;
            v = Utils::STATE5::S5_1;
        }
        else
        {
            cnt0 = CNT0;
            cnt1 = 0;
            v = Utils::STATE5::S5_0;
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
                Set();
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
                Clr();
            }
        }
    }

    void ResetCnt()
    {
        cnt0 = 0;
        cnt1 = 0;
    }

private:

    int CNT1{1};
    int CNT0{1};
    int cnt1{0};
    int cnt0{0};
};
