#ifndef __IPEROIDICRUN_H__
#define __IPEROIDICRUN_H__

class IPeriodicRun
{
public:
    virtual ~IPeriodicRun(){};

    virtual void PeriodicRun()=0;
};

#endif
