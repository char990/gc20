#include <stdexcept>

#include "TsiSp003.h"

TsiSp003::TsiSp003()
{
    sessionTimeout.Setms(-1);
    displayTimeout.Setms(-1);
    int n = GetFreeSpace();
    if(n==-1)
    {
        throw std::overflow_error("TsiSp003 space full");
    }
    tsiSp003s[n]=this;
}

TsiSp003::~TsiSp003()
{
    for(int i=0;i<MAX_TsiSp003;i++)
    {
        if(tsiSp003s[i]==this)
        {
            tsiSp003s[i]=nullptr;
            return;
        }
    }
}

int TsiSp003::GetFreeSpace()
{
    for(int i=0;i<MAX_TsiSp003;i++)
    {
        if(tsiSp003s[i]==nullptr)
        {
            return i;
        }
    }
    return -1;
}

void TsiSp003::SetCfg(TsiSp003Cfg *cfg)
{
    tsiSp003Cfg = cfg;
}

void TsiSp003::Run()
{

}

void TsiSp003::SessionTimeout()
{

}

void TsiSp003::DisplayTimeout()
{

}

