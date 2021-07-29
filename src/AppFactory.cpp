#include "AppFactory.h"
#include "TsiSp003AppVer10.h"
#include "TsiSp003AppVer31.h"
#include "TsiSp003AppVer50.h"
#include "DbHelper.h"

AppFactory::AppFactory(bool & online)
{
    auto ver = dbHelper.TsiSp003Ver();
    if(ver==0x10)
    {
        app = new TsiSp003AppVer10(online);
    }
    if(ver==0x31)
    {
        app = new TsiSp003AppVer31(online);
    }
    else    //    if(ver==0x50) default
    {
        app = new TsiSp003AppVer50(online);
    }
}

AppFactory::~AppFactory()
{
    delete app;
}

