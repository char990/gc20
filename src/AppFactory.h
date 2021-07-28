#ifndef __APPFACTORY_H__
#define __APPFACTORY_H__

#include <string>
#include "ILayer.h"
#include "TsiSp003App.h"


/// \brief TSiSp003 Application Layer agent
class AppFactory
{
public:
    AppFactory(bool & online);
    ~AppFactory();

    TsiSp003App * GetApp(){ return app; };

private:
    TsiSp003App * app;
};

#endif
