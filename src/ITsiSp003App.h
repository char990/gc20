#ifndef __ITSISP003App_H__
#define __ITSISP003App_H__

#include <string>
#include "IAppAdaptor.h"

class ITsiSp003App
{
public:
    virtual std::string Version()=0;

    /// \brief Receiving Handle, called by LowerLayer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: unknown command
    virtual int Rx(uint8_t * data, int len)=0;

    /// \brief Sending data to adaptor
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         -1: failed; >=0: ms for sending out
    virtual int Tx(uint8_t * data, int len)
    {
        if(adaptor!=nullptr)
        {
            return adaptor->Tx(data,len);
        }
        return -1;
    }

    /// \brief  Check and run new added MI rather than old revision
    /// \param  data    data buffer
    /// \param  len     data len
    /// \return int     -1: No cmd matched, call base Rx()
    virtual int NewMi(uint8_t * data, int len)=0;

    /// \brief set adaptor for Tx()
    virtual void SetAdaptor(IAppAdaptor * adp)
    {
        adaptor=adp;
    }

protected:
    IAppAdaptor * adaptor;
};

#endif
