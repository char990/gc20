#include <tsisp003/AppFactory.h>
#include <tsisp003/TsiSp003AppVer10.h>
#include <tsisp003/TsiSp003AppVer31.h>
#include <tsisp003/TsiSp003AppVer50.h>



AppFactory::AppFactory()
{
    auto ver = DbHelper::Instance().uciProd.TsiSp003Ver();
    if(ver==0)
    {
        app = new TsiSp003AppVer10();
    }
    else if(ver==1)
    {
        app = new TsiSp003AppVer31();
    }
    else if(ver=2)
    {
        app = new TsiSp003AppVer50();
    }
    else
    {
        MyThrow("Unknow TsiSp003Ver %d",ver);
    }
}

AppFactory::~AppFactory()
{
    delete app;
}

