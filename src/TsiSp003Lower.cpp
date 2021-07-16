#include <stdexcept>

#include "TsiSp003Lower.h"

TimerEvent * TsiSp003Lower::tmrEvent = nullptr;

TsiSp003Lower::TsiSp003Lower()
{
    int n = GetFreeSpace();
    if(n==-1)
    {
        throw std::overflow_error("TsiSp003Lower space full");
    }
    sessionTimeout.Setms(-1);
    displayTimeout.Setms(-1);
    tmrEvent->Add(this);
}

TsiSp003Lower::~TsiSp003Lower()
{
    tmrEvent->Remove(this);
}

int TsiSp003Lower::GetFreeSpace()
{
    
}

void TsiSp003Lower::PeriodicRun()
{

}

void TsiSp003Lower::SessionTimeout()
{

}

void TsiSp003Lower::DisplayTimeout()
{

}

void TsiSp003Lower::IncNr()
{
    nr++;
    if(nr==0)nr=1;
}

void TsiSp003Lower::IncNs()
{
    ns++;
    if(ns==0)ns=1;
}

void TsiSp003Lower::Rx()
{

}

void TsiSp003Lower::Tx()
{

}
