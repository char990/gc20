#include <tsisp003/AppFactory.h>
#include <tsisp003/QLD21.h>
#include <tsisp003/NSW31.h>
#include <tsisp003/NSW50.h>
#include <module/Utils.h>



AppFactory::AppFactory()
{
    auto ver = DbHelper::Instance().GetUciHardware().TsiSp003Ver();
    if(ver==0)
    {
        app = new QLD21();
    }
    else if(ver==1)
    {
        app = new NSW31();
    }
    else if(ver=2)
    {
        app = new NSW50();
    }
    else
    {
        throw std::invalid_argument(Utils::StrFn::PrintfStr("Unknow TsiSp003Ver %d",ver));
    }
}

AppFactory::~AppFactory()
{
    delete app;
}

