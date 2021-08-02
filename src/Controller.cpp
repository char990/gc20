#include "Controller.h"

Controller::Controller()
:signs(nullptr)
{
}

Controller::~Controller()
{
    if(signs != nullptr)
    {
        delete [] signs;
    }
    if(groups != nullptr)
    {
        delete [] groups;
    }
}

void Controller::Init()
{
    signCnt=4;
    displayTimeout.Clear();
    signs = new Sign[signCnt];
    for(int i=0;i<signCnt;i++)
    {
        signs[i].SetId(i+1, (i&1)+1);
    }
    groups = new SignsInGroup[2]{SignsInGroup(2),SignsInGroup(2)};
    int ct[2];
    ct[0]=0;
    ct[1]=0;
    for(int i=0;i<signCnt;i++)
    {
        if(signs[i].GroupId()==1)
        {
            groups[0].signsInGrp[ct[0]]=&signs[i];
            ct[0]++;
        }
        else if(signs[i].GroupId()==2)
        {
            groups[0].signsInGrp[ct[1]]=&signs[i];
            ct[1]++;
        }
    }
}

void Controller::RefreshDispTime()
{
    displayTimeout.Setms(100000);
}

void Controller::SessionLed(uint8_t v)
{
    sessionLed = v;
}

uint8_t Controller::ErrorCode()
{
    return errorCode;
}

uint8_t Controller::SignCnt()
{
    return signCnt;
}

