#include "AppFactory.h"
#include "TsiSp003AppVer10.h"
#include "TsiSp003AppVer31.h"
#include "TsiSp003AppVer50.h"

AppFactory::AppFactory(bool & online)
{
    int ver=50;
    if(ver==10)
    {
        app = new TsiSp003AppVer10(online);
    }
    if(31)
    {
        app = new TsiSp003AppVer31(online);
    }
    else    //    if(50) default
    {
        app = new TsiSp003AppVer50(online);
    }
}

AppFactory::~AppFactory()
{
    delete app;
}

