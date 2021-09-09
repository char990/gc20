#ifndef __GROUPAPP_H__
#define __GROUPAPP_H__

#include <string>
#include <layer/ILayer.h>

/// \brief 
class GroupApp : public IUpperLayer
{
public:
    GroupApp();
    virtual ~GroupApp();

    /*<------------------------------------------------------------------*/
    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief		Clean current layer. Called by lowerlayer->Clean() and call upperlayer->Clean()
    virtual void Clean() override;
    /*------------------------------------------------------------------>*/

    /// \brief call lowerlayer->Tx
    virtual int Tx(uint8_t * data, int len) { return lowerLayer->Tx(data, len); }

protected:

};

#endif
