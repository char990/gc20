#ifndef __TSISP003App_H__
#define __TSISP003App_H__

#include <string>
#include "ILayer.h"
#include "IOnline.h"

/// \brief TSiSp003 Application Layer base
class TsiSp003App : public IUpperLayer, public IOnline
{
public:
    TsiSp003App();
    virtual ~TsiSp003App();

    virtual std::string Version()=0;

    /*<------------------------------------------------------------------*/
    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief		Clean current layer. Called by lowerlayer->Clean() and call upperlayer->Clean()
    virtual void Clean() override;
    /*------------------------------------------------------------------>*/

    /*<------------------------------------------------------------------*/
    /// \brief		Online status, provide to session layer
    virtual bool Online() override { return online; };
    virtual void Online(bool v) override { online=v; };
    /*------------------------------------------------------------------>*/


    /*<------------------------------------------------------------------*/
    /// TsiSp003App is base of App layer, only implement basic commands

    /// \brief  Reject
    void Reject(uint8_t error);

    /// \brief  Acknowledge
    void Ack();
    
    /// \brief      Check length, if not matched, Reject
    bool ChkLen(int len1, int len2);
    /*------------------------------------------------------------------>*/

    /// \brief call lowerlayer->Tx
    virtual int Tx(uint8_t * data, int len) { return lowerLayer->Tx(data, len); }

protected:
    bool online;
    uint8_t startSession;
    uint16_t password;
    uint8_t seed;
    uint8_t micode;

    void StartSession(uint8_t * data, int len);
    void Password(uint8_t * data, int len);
    void EndSession(uint8_t * data, int len);
    void UpdateTime(uint8_t * data, int len);
    void SignSetDimmingLevel(uint8_t * data, int len);
    void PowerONOFF(uint8_t * data, int len);
    void DisableEnableDevice(uint8_t * data, int len);
    void RetrieveFaultLog(uint8_t * data, int len);
    void ResetFaultLog(uint8_t * data, int len);

    /// \brief  Make password from seed
    uint16_t MakePassword ();

};

#endif
