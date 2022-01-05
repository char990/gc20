#pragma once

class DispStatus
{
public:
    DISP_TYPE dispType;
    uint8_t *fmpid;
    uint8_t signCnt;

    DispStatus(uint8_t signCnt)
    :signCnt(signCnt)
    {
        fmpid = new uint8_t[signCnt];
        Clear();
    };

    ~DispStatus()
    {
        delete [] fmpid;
    };
    
    void Clone(DispStatus * ds)
    {
        dispType = ds->dispType;
        for(int i=0;i<signCnt;i++)
        {
            fmpid[i] = ds->fmpid[i];
        }
    };
    
    void Frm0()
    {
        dispType = DISP_TYPE::FRM;
        fmpid[0] = 0;
    };
    
    void Pln0()
    {
        dispType = DISP_TYPE::PLN;
        fmpid[0] = 0;
    };
    
    void BLK()
    {
        dispType = DISP_TYPE::BLK;
        fmpid[0] = 0;
    };
    
    void N_A()
    {
        dispType = DISP_TYPE::N_A;
        fmpid[0] = 0;
    };

    void Clear()
    {
        dispType = DISP_TYPE::N_A;
        memset(fmpid, 0, signCnt);
    }
    
    bool Equal(DispStatus * dst)
    {
        if(dispType == dst->dispType && signCnt == dst->signCnt)
        {
            if(dispType == DISP_TYPE::ATF)
            {
                for(int i=0;i<signCnt;i++)
                {
                    if(fmpid[i] != dst->fmpid[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            else if(dispType == DISP_TYPE::N_A || dispType == DISP_TYPE::BLK)
            {
                return true;
            }
            else
            {
                return (fmpid[0] == dst->fmpid[0]);
            }
        }
        return false;
    };
};

