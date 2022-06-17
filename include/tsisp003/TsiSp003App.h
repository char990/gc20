#pragma once

#include <string>
#include <layer/ILayer.h>
#include <tsisp003/TsiSp003Const.h>
#include <uci/DbHelper.h>
#include <sign/Controller.h>
#include <layer/ISession.h>
#include <tsisp003/Upgrade.h>

/// \brief TSiSp003 Application Layer base
class TsiSp003App : public IUpperLayer
{
public:
    TsiSp003App();
    virtual ~TsiSp003App();

    virtual std::string Version() = 0;

    /*<------------------------------------------------------------------*/
    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t *data, int len) override;

    /// \brief		ClrRx current layer. Called by lowerlayer->ClrRx() and call upperlayer->ClrRx()
    virtual void ClrRx() override;
    /*------------------------------------------------------------------>*/

    /*<------------------------------------------------------------------*/
    /// TsiSp003App is base of App layer, only implement basic commands

    /// \brief  Ack or Reject by r
    void AckRjct(APP::ERROR r) { (r == APP::ERROR::AppNoError) ? Ack() : Reject(r); };

    /// \brief  Reject
    void Reject(APP::ERROR error);
    void SetRejectStr(const char *fmt, ...);

    /// \brief  Acknowledge
    void Ack();

    /// \brief      Check length, if not matched, Reject
    bool ChkLen(int rcvd, int expect);

    /// \brief      Check online status
    bool IsOnline();

    /// \brief      Reject if offline
    bool CheckOnline_RejectIfFalse();

    /*------------------------------------------------------------------>*/

    /// \brief call lowerlayer->Tx
    virtual int Tx(uint8_t *data, int len) { return lowerLayer->Tx(data, len); }

    void SetSession(ISession *v) { session = v; };

protected:
    DbHelper &db;
    Controller &ctrller;
    ISession *session{nullptr};

    uint8_t micode{0};
    APP::ERROR appErr{APP::ERROR::AppNoError};

    uint8_t txbuf[MAX_APP_PACKET_SIZE];

    void StartSession(uint8_t *data, int len);
    void Password(uint8_t *data, int len);
    void EndSession(uint8_t *data, int len);

    /// \brief  Make password from seed
    uint16_t MakePassword();

    char rejectStr[64];

private:
    // all UserDefinedCmd functions are in UserDefinedCmd.cpp
    // FACMD list
    enum FACMD : uint8_t
    {
        FACMD_SET_LUMINANCE = 0x01,
        FACMD_SET_EXT_INPUT = 0x02,
        FACMD_RQST_EXT_INPUT = 0x03,
        FACMD_RQST_LUMINANCE = 0x04,

        FACMD_RTRV_LOGS = 0x0A,
        FACMD_RPL_FLT_LOGS = 0x0B,
        FACMD_RPL_ALM_LOGS = 0x0C,
        FACMD_RPL_EVT_LOGS = 0x0D,
        FACMD_RESET_LOGS = 0x0F,

        FACMD_SEND_FINFO = 0x10,
        FACMD_SEND_FPKT = 0x11,
        FACMD_START_UPGRD = 0x12,

        FACMD_SET_USER_CFG = 0x20,
        FACMD_RQST_USER_CFG = 0x21,
        FACMD_RQST_USER_EXT = 0x22,
        FACMD_RPL_USER_EXT = 0x23,
        FACMD_SET_SIGN_CFG = 0x24,
        FACMD_RPL_SET_USER_CFG = 0x25,

        FACMD_SHAKE_RQST = 0xF0,
        FACMD_SHAKE_REPLY = 0xF1,
        FACMD_SHAKE_PASSWD = 0xF2,
        FACMD_RESTART = 0xF5,
        FACMD_REBOOT = 0xFA
    };

    int shake_hands_status{0};
    uint8_t shake_src[26];  // 16-byte salt + 10-byte password 

    int UserDefinedCmd(uint8_t *data, int len);
    int UserDefinedCmdFA(uint8_t *data, int len);
    int FA01_SetLuminance(uint8_t *data, int len);
    int FA02_SetExtInput(uint8_t *data, int len);
    int FA03_RqstExtInput(uint8_t *data, int len);
    int FA04_RqstLuminance(uint8_t *data, int len);
    int FA0A_RetrieveLogs(uint8_t *data, int len);
    int FA0F_ResetLogs(uint8_t *data, int len);
    
    int FA10_SendFileInfo(uint8_t *data, int len);
    int FA11_SendFilePacket(uint8_t *data, int len);
    int FA12_StartUpgrading(uint8_t *data, int len);

    int FA20_SetUserCfg(uint8_t *data, int len);
    int FA21_RqstUserCfg(uint8_t *data, int len);
    int FA22_RqstUserExt(uint8_t *data, int len);

    int FAF0_ShakehandsRqst(uint8_t *data, int len);
    int FAF2_ShakehandsPasswd(uint8_t *data, int len);
    int FAF5_Restart(uint8_t *data, int len);
    int FAFA_Reboot(uint8_t *data, int len);

    void Md5_of_sh(const char *str, unsigned char *md5);
    APP::ERROR CheckFA20(uint8_t *pd, char * shake);

    Upgrade upgrade;


    int FE_SetGuiConfig(uint8_t *data, int len);
    int FF_RqstGuiConfig(uint8_t *data, int len);
};
