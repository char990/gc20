#include <module/FacilitySwitch.h>
#include <gpio/GpioIn.h>
#include <module/MyDbg.h>

FacilitySwitch::FacilitySwitch(uint32_t pinAuto, uint32_t pinM1, uint32_t pinM2)
:cnt{0}
{
    pFsAuto = new GpioIn(5, 10, pinAuto);
    pFsM1 = new GpioIn(5, 10, pinM1);
    pFsM2 = new GpioIn(5, 10, pinM2);
}

FacilitySwitch::~FacilitySwitch()
{
    delete pFsM2;
    delete pFsM1;
    delete pFsAuto;
}

void FacilitySwitch::PeriodicRun()
{
    if(++cnt<10)    // every 10*10ms
    {
        return;
    }
    cnt=0;
    pFsAuto->PeriodicRun();
    pFsM1->PeriodicRun();
    pFsM2->PeriodicRun();
    if (pFsAuto->IsChanged() || pFsM1->IsChanged() || pFsM2->IsChanged())
    {
        auto fsauto = pFsAuto->Value();
        auto fsm1 = pFsM1->Value();
        auto fsm2 = pFsM2->Value();
        if (fsauto != Utils::STATE3::S_NA && fsm1 != Utils::STATE3::S_NA && fsm2 != Utils::STATE3::S_NA)
        {
            pFsAuto->ClearChanged();
            pFsM1->ClearChanged();
            pFsM2->ClearChanged();
            uint8_t key = 0;
            if (fsauto == Utils::STATE3::S_1)
            {
                key |= 1;
            }
            if (fsm1 == Utils::STATE3::S_1)
            {
                key |= 2;
            }
            if (fsm2 == Utils::STATE3::S_1)
            {
                key |= 4;
            }
            switch (key)
            {
            case 0x03: // 011:M2
                isChanged = true;
                fsState = FS_STATE::MSG2;
                PrintDbg("FacilitySwitch:MSG2\n");
                break;
            case 0x05: // 101:M1
                isChanged = true;
                fsState = FS_STATE::MSG1;
                PrintDbg("FacilitySwitch:MSG1\n");
                break;
            case 0x06: // 110:AUTO
                isChanged = true;
                fsState = FS_STATE::AUTO;
                PrintDbg("FacilitySwitch:AUTO\n");
                break;
            case 0x07: // 111:OFF
                isChanged = true;
                fsState = FS_STATE::OFF;
                PrintDbg("FacilitySwitch:OFF\n");
                break;
            default: // invalid
                break;
            }
        }
    }
}
