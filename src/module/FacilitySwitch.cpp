#include <module/FacilitySwitch.h>
#include <gpio/GpioIn.h>
#include <module/MyDbg.h>
#include <tsisp003/TsiSp003Const.h>

FacilitySwitch::FacilitySwitch(uint32_t pinAuto, uint32_t pinM1, uint32_t pinM2)
{
    pFsAuto = new GpioIn(CTRLLER_MS(200), CTRLLER_MS(200), pinAuto);
    pFsM1 = new GpioIn(CTRLLER_MS(200), CTRLLER_MS(200), pinM1);
    pFsM2 = new GpioIn(CTRLLER_MS(200), CTRLLER_MS(200), pinM2);
}

FacilitySwitch::~FacilitySwitch()
{
    delete pFsM2;
    delete pFsM1;
    delete pFsAuto;
}

void FacilitySwitch::PeriodicRun()
{
    pFsAuto->PeriodicRun();
    pFsM1->PeriodicRun();
    pFsM2->PeriodicRun();
    uint8_t key = 0;
    if (pFsAuto->IsValid() && pFsM1->IsValid() && pFsM2->IsValid())
    {
        if (pFsAuto->IsHigh())
        {
            key |= 1;
        }
        if (pFsM1->IsHigh())
        {
            key |= 2;
        }
        if (pFsM2->IsHigh())
        {
            key |= 4;
        }
    }
    else
    {
        return;
    }
    if (lastkey != key)
    {
        keyCnt = 0;
        lastkey = key;
        return;
    }
    if (++keyCnt == 10)
    {
        keyCnt = 0;
        switch (key)
        {
        case 0x03: // 011:M2
            fsState = FS_STATE::MSG2;
            break;
        case 0x05: // 101:M1
            fsState = FS_STATE::MSG1;
            break;
        case 0x06: // 110:AUTO
            fsState = FS_STATE::AUTO;
            break;
        case 0x07: // 111:OFF
            fsState = FS_STATE::OFF;
            break;
        default: // invalid
            break;
        }
        if (lastState != fsState)
        {
            sprintf(buf, "FacilitySwitch:%s->%s", FS_STR[lastState], FS_STR[fsState]);
            isChanged = true;
            lastState = fsState;
        }
    }
}
