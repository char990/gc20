#pragma once

#include <string>
#include <layer/ILayer.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Scheduler.h>
#include <layer/ISession.h>


/// \brief TSiSp003 Application Layer base
class TsiSp003App : public IUpperLayer
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

    /// \brief		ClrRx current layer. Called by lowerlayer->ClrRx() and call upperlayer->ClrRx()
    virtual void ClrRx() override;
    /*------------------------------------------------------------------>*/

    /*<------------------------------------------------------------------*/
    /// TsiSp003App is base of App layer, only implement basic commands

    /// \brief  Ack or Reject by r
    void AckRjct(APP::ERROR r) { (r==APP::ERROR::AppNoError)?Ack():Reject(r); };

    /// \brief  Reject
    void Reject(APP::ERROR error);

    /// \brief  Acknowledge
    void Ack();
    
    /// \brief      Check length, if not matched, Reject
    bool ChkLen(int len1, int len2);

    /// \brief      Check online status
    bool IsOnline();

    /// \brief      Reject if offline
    bool CheckOlineReject();

    /*------------------------------------------------------------------>*/

    /// \brief call lowerlayer->Tx
    virtual int Tx(uint8_t * data, int len) { return lowerLayer->Tx(data, len); }

    void SetSession(ISession * v) { session = v; };

protected:
    DbHelper & db;
    Scheduler & sch;
    ISession *session{nullptr};

    uint8_t micode{0};
    APP::ERROR appErr{APP::ERROR::AppNoError};
    

    uint8_t txbuf[MAX_APP_PACKET_SIZE];

    void StartSession(uint8_t * data, int len);
    void Password(uint8_t * data, int len);
    void EndSession(uint8_t * data, int len);
    void UpdateTime(uint8_t * data, int len);

    /// \brief  Make password from seed
    uint16_t MakePassword ();

private:
    // all UserDefinedCmd functions are in UserDefinedCmd.cpp
    int UserDefinedCmd(uint8_t *data, int len);
    int UserDefinedCmdFA(uint8_t *data, int len);
    int FA0A_RetrieveLogs(uint8_t *data, int len);
    int FA0F_ResetLogs(uint8_t *data, int len);
};

