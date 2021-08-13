#ifndef __APPFACTORY_H__
#define __APPFACTORY_H__

#include <string>
#include <layer/ILayer.h>
#include <tsisp003/TsiSp003App.h>


/// \brief TSiSp003 Application Layer agent
class AppFactory
{
public:
    AppFactory();
    ~AppFactory();

    TsiSp003App * GetApp(){ return app; };

private:
    TsiSp003App * app;
};

#endif
