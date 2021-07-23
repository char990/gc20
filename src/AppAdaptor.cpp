#include <stdexcept>
#include "AppAdaptor.h"
#include "Phcs2AppAdaptor.h"
#include "Web2AppAdaptor.h"

AppAdaptor::AppAdaptor(std::string name, std::string aType, IOperator * iOperator)
{
    if(aType.compare("PHCS")==0)
    {
        adaptor = new Phcs2AppAdaptor(name, iOperator);
    }
    else if(aType.compare("WEB")==0)
    {
        adaptor = new Web2AppAdaptor(name, iOperator);
    }
    else
    {
        throw std::invalid_argument("Unkown adaptor type:"+aType);
    }
}

AppAdaptor::~AppAdaptor()
{
    delete adaptor;
}

int AppAdaptor::Rx(int fd)
{
    return adaptor->Rx(fd);
}

int AppAdaptor::Tx(uint8_t * data, int len)
{
    return adaptor->Tx(data,len);
}

void AppAdaptor::PeriodicRun()
{
    return adaptor->PeriodicRun();
}
