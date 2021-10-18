#include <module/ExtInput.h>

ExtInput::ExtInput(uint32_t pinM3,uint32_t pinM4,uint32_t pinM5)
:cnt{0}
{
    pExtM3 = new GpioIn(2, 2, pinM3);
    pExtM4 = new GpioIn(2, 2, pinM4);
    pExtM5 = new GpioIn(2, 2, pinM5);
}

ExtInput::~ExtInput() 
{
    delete pExtM3;
    delete pExtM4;
    delete pExtM5;
}

void ExtInput::PeriodicRun() 
{
    if(++cnt<10)    // every 10*10ms
    {
        return;
    }
    cnt=0;
    pExtM3->PeriodicRun();
    pExtM4->PeriodicRun();
    pExtM5->PeriodicRun();    
}

void ExtInput::Reset()
{
    
}
