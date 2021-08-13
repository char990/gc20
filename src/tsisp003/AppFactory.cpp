#include <tsisp003/AppFactory.h>
#include <tsisp003/TsiSp003AppVer10.h>
#include <tsisp003/TsiSp003AppVer31.h>
#include <tsisp003/TsiSp003AppVer50.h>
#include <uci/DbHelper.h>

AppFactory::AppFactory()
{
    auto ver = DbHelper::Instance().prodCfg.TsiSp003Ver();
    if(ver==0x10)
    {
        app = new TsiSp003AppVer10();
    }
    if(ver==0x31)
    {
        app = new TsiSp003AppVer31();
    }
    else    //    if(ver==0x50) default
    {
        app = new TsiSp003AppVer50();
    }
}

AppFactory::~AppFactory()
{
    delete app;
}

