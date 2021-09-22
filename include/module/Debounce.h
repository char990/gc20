#ifndef __DEBOUNCE_H__
#define __DEBOUNCE_H__

#include <cstdint>
#include <cstdio>

typedef enum {F_NA=-1,F_CLEAR=0,F_ONSET=1} fault_t;


/// \brief bool debounce
/// start at invalid
/// if true over true_cnt, valid, value=true
/// if false over false_cnt, valid, value=false
class Debounce
{
public:
    Debounce(int true_cnt,  int false_cnt)
    {
        SetCNT(true_cnt,false_cnt);
        Reset();
    }

    /// \breif Reset debounce state
    /// clear counter, invalid state, keep value
    void Reset()
    {
        cnt0=0;
        cnt1=0;
        is_valid = false;
        changed = false;
    }

    /// \breif Reset debounce state
    /// true    : valid & changed, value = false
    /// false   : valid & changed, value = true
    void SetState(bool v)
    {
        if(v)
        {
            cnt0=0;
            cnt1=CNT1;
            is_valid = true;
            changed = true;
            value = true;
        }
        else
        {
            cnt0=CNT0;
            cnt1=0;
            is_valid = true;
            changed = true;
            value = false;
        }
    }

    /// \brief  Set counter, should follow Reset/SetState
    void SetCNT(int true_cnt,  int false_cnt)
    {
        CNT1=true_cnt;
        CNT0=false_cnt;
    };

    // Called regularly
    void Check(bool v)
    {
        if(v)
        {
            if(cnt1<CNT1)
            {
                cnt1++;
            }
            if(cnt1>=CNT1)
            {
                cnt0=0;
                cnt1=CNT1;
                is_valid=true;
                changed = (value==false);
                value=true;
            }
        }
        else
        {
            if(cnt0<CNT0)
            {
                cnt0++;
            }
            if(cnt0>=CNT0)
            {
                cnt1=0;
                cnt0=CNT0;
                is_valid=true;
                changed = (value==true);
                value=false;
            }
        }
    }

    void ResetCnt()
    {
        cnt0=0;
        cnt1=0;
    }

    bool changed=false;
    bool IsValid(){return is_valid;}

    bool Value(){return value;}

    fault_t Fault(void)
    {
        if(!is_valid)
        {
            return F_NA;
        }
        return value?F_ONSET:F_CLEAR;
    };
    
private:
    bool is_valid;
    bool value;
    int CNT1;
    int CNT0;
    int cnt1;
    int cnt0;
};

#endif
