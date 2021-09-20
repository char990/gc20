#ifndef __UNITEDSIGN_H__
#define __UNITEDSIGN_H__

#include <cstdint>

class DispStatus
{
public:
    DISP_STATUS::TYPE dispType;
    uint8_t fmpid;
};

class UnitedSign
{
public:
    UnitedSign(uint8_t id)
    :signId(id),power(0),
    dimmingSet(0),dimmingV(1),
    deviceSet(1),deviceV(1)
    {};
    virtual ~UnitedSign(){};

    uint8_t SignId() {return signId;};

    virtual void Reset()=0;
    virtual void PeriodicRun() = 0;
    virtual uint8_t * GetExtStatus(uint8_t *p)=0;
//    virtual void DispBackup()=0;


    uint8_t * GetStatus(uint8_t *p);

    void DispNext(DISP_STATUS::TYPE type,uint8_t id);

    void SetDimming(uint8_t v) { dimmingSet = v; };
    void SetPower(uint8_t v) { power = v; };
    void SetDevice(uint8_t v) { deviceSet = v; };

protected:
    uint8_t signId, power;
    uint8_t dimmingSet, dimmingV;
    uint8_t deviceSet, deviceV;

    DispStatus dsBak;
    DispStatus dsCurrent;
    DispStatus dsNext;
    DispStatus dsExt;

    uint8_t currentPln, currentMsg, currentFrm;
};

#endif
