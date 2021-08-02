#include "AppFactory.h"
#include "TsiSp003AppVer10.h"
#include "TsiSp003AppVer31.h"
#include "TsiSp003AppVer50.h"
#include "DbHelper.h"

AppFactory::AppFactory()
{
    auto ver = dbHelper.TsiSp003Ver();
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

