#pragma once

class DispStatus
{
public:
    DISP_STATUS::TYPE dispType;
    uint8_t *fmpid;
    uint8_t signCnt;

    DispStatus(uint8_t signCnt)
    :signCnt(signCnt)
    {
        dispType = DISP_STATUS::TYPE::N_A;
        fmpid = new uint8_t[signCnt];
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
        dispType = DISP_STATUS::TYPE::FRM;
        fmpid[0] = 0;
    };
    
    void Pln0()
    {
        dispType = DISP_STATUS::TYPE::PLN;
        fmpid[0] = 0;
    };
    
    void N_A()
    {
        dispType = DISP_STATUS::TYPE::N_A;
        fmpid[0] = 0;
    };
    
    bool Equal(DispStatus * dst)
    {
        if(dispType == dst->dispType && signCnt == dst->signCnt)
        {
            if(dispType == DISP_STATUS::TYPE::ATF)
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
            else if(dispType == DISP_STATUS::TYPE::N_A || dispType == DISP_STATUS::TYPE::BLK)
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

