#ifndef __WEB2APPADAPTOR_H__
#define __WEB2APPADAPTOR_H__
#include <string>
#include "IAppAdaptor.h"
#include "IOperator.h"
#include "TsiSp003App.h"
class Web2AppAdaptor : public IAppAdaptor
{
public:
    Web2AppAdaptor(std::string name, IOperator * iOperator);
    ~Web2AppAdaptor();

    /// \brief		periodic run
    virtual void PeriodicRun() override;

    /// \brief		data received
    /// \param      int fd : file desc
    /// \return     -1: Error; 0:Closed; n:bytes
    virtual int Rx(int fd) override;
    
    /// \brief Transmitting function, call Tx() of operator
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len) override;

private:
    std::string name;
    IOperator * iOperator;
    TsiSp003App *app;
};

#endif
