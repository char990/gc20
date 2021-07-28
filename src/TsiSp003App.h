#ifndef __TSISP003App_H__
#define __TSISP003App_H__

#include <string>
#include "ILayer.h"

enum MiCode
{
    Reject = 0x00,
    ACK,
    StartSession,
    PasswordSeed,
    Password,
    HeartbeatPoll,
    SignStatusReply,
    EndSession,
    SystemReset,
    UpdateTime,
    SignSetTextFrame,
    SignSetGraphicsFrame,
    SignSetMessage,
    SignSetPlan,
    SignDisplayFrame,
    SignDisplayMessage,
    EnablePlan,
    DisablePlan,
    RequestEnabledPlans,
    ReportEnabledPlans,
    SignSetDimmingLevel,
    PowerONOFF,
    DisableEnableDevice,
    SignRequestStoredFMP,
    RetrieveFaultLog,
    FaultLogReply,
    ResetFaultLog,
    SignExtendedStatusRequest,
    SignExtendedStatusReply,
    HARStatusReply = 0x40,
    HARSetVoiceDataIncomplete,
    HARSetVoiceDataComplete,
    HARSetStrategy,
    HARActivateStrategy,
    HARSetPlan,
    HARRequestStoredVSP,
    HARSetVoiceDataACK,
    HARSetVoiceDataNAK,
    EnvironmentalWeatherStatusReply = 0x80,
    RequestEnvironmentalWeatherValues,
    EnvironmentalWeatherValues,
    EnvironmentalWeatherThresholdDefinition,
    RequestThresholdDefinition,
    RequestEnvironmentalWeatherEventLog,
    EnvironmentalWeatherEventLogReply,
    ResetEnvironmentalWeatherEventLog,
    UserDefinedCmdF0 = 0xF0,
    UserDefinedCmdF1,
    UserDefinedCmdF2,
    UserDefinedCmdF3,
    UserDefinedCmdF4,
    UserDefinedCmdF5,
    UserDefinedCmdF6,
    UserDefinedCmdF7,
    UserDefinedCmdF8,
    UserDefinedCmdF9,
    UserDefinedCmdFA,
    UserDefinedCmdFB,
    UserDefinedCmdFC,
    UserDefinedCmdFD,
    UserDefinedCmdFE,
    UserDefinedCmdFF
};

typedef void (*AppFun)();
class Cmd
{
public:
    Cmd(uint8_t mi, AppFun cmd):mi(mi),cmd(cmd){};
    uint8_t mi;
    AppFun cmd;
};

/// \brief TSiSp003 Application Layer base
class TsiSp003App : public ILayer
{
public:
    TsiSp003App(bool & online);
    virtual ~TsiSp003App();

    virtual std::string Version()=0;

    /// \brief Transmitting function, called by upperlayer and call lowerlayer->Tx()
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         time in ms for sending all data
    virtual int Tx(uint8_t * data, int len) override;

    /// \brief Receiving Handle, called by Lower-Layer
    /// \param		data		data buffer
    /// \param		len		    data length
    /// \return     int         0: Command excuted; -1: failed
    virtual int Rx(uint8_t * data, int len) override;

    /// \brief		Release current layer. Called by upperlayer and call lowerlayer->Release()
    virtual void Release() override;

    /// \brief		Periodic run. Called by lowerlayer->PeriodicRun() and call upperlayer->PeriodicRun()
    virtual void PeriodicRun() override;

    /// \brief		Init current layer. Called by lowerlayer->Init() and call upperlayer->Init()
    virtual void Clean() override;

    /// TsiSp003App is abse of App layer, only implement StartSession, Password & EndSession
    
protected:
    bool & online;
    uint16_t password;
    uint8_t seed;
    
    Cmd baseCmds[]=
    {
        Cmd(MiCode::StartSession, &TsiSp003App::StartSession),
        Cmd(MiCode::Password, &TsiSp003App::Password),
        Cmd(MiCode::EndSession, &TsiSp003App::EndSession)
    };
    /// \brief  Start Session
    void StartSession();

    /// \brief  Password
    void Password();

    /// \brief  End Session
    void EndSession();
  

};

#endif
