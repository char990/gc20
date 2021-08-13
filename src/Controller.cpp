#include <module/Controller.h>
#include <module/SignHrg.h>
#include <module/SignGfx.h>
#include <module/SignTxt.h>


Controller::Controller()
:signs(nullptr)
{
}

Controller::~Controller()
{
    if(signs != nullptr)
    {
        for(int i=0;i<signCnt;i++)
        {
            delete signs[i];
        }
        delete [] signs;
    }
}

void Controller::Init()
{
    displayTimeout.Clear();
    signCnt=4;
    signs = new Sign * [signCnt];
    for(int i=0;i<signCnt;i++)
    {
        //auto s = ;
        signs[i] = new SignTxt();
    }
    signs[0]->SetId(1, 1);
    signs[1]->SetId(2, 1);
    signs[2]->SetId(3, 2);
    signs[3]->SetId(4, 2);
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

