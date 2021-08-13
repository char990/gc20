#ifndef __TSISP003CONST_H__
#define __TSISP003CONST_H__

namespace MI
{
    enum CODE
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
        SignSetHighResolutionGraphicsFrame,
        SignConfigurationRequest = 0x21,
        SignConfigurationReply,
        SignDisplayAtomicFrames = 0x2B,
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
}
namespace APP
{
    enum ERROR
    {
        AppNoError = 0,
        DeviceControllerOffline,
        SyntaxError,
        LengthError,
        DataChksumError,
        TextNonASC,
        FrameTooLarge,
        UnknownMi,
        MiNotSupported,
        PowerIsOff,
        UndefinedDeviceNumber,
        FontNotSupported,
        ColourNotSupported,
        OverlaysNotSupported,
        DimmingLevelNotSupported,
        FrmMsgPlnActive,
        FacilitySwitchOverride,
        ConspicuityNotSupported,
        TransitionTimeNotSupported,
        FrmMsgPlnUndefined,
        PlanNotEnabled,
        PlanEnabled,
        SizeMismatch,
        FrameTooSmall,
        HARStrategyStopped,
        HarStrategyUndefined,
        HARStrategyError,
        HARVoiceError,
        HARVoiceNotSupported,
        HARHardwareError,
        TimeExpired,
        CollourDepthNotSupported,
        IncompleteColouFrameDefinition,
        Incorrect_Password,
        Interlocking_Invalid_Setting,
        Interlocking_Missing_Signs,
        Interlocking_Not_Active,
        Interloccking_Active
    };

}

namespace DEV
{
    enum ERROR
    {
        DevNoError = 0,
        PowerFailure,
        CommunicationsTimeoutError,
        MemoryError,
        BatteryFailure,
        InternalCommunicationsFailure,
        SignLampFailure,
        SignSingleLedFailure,
        SignMultiLedFailure,
        OverTemperatureAlarm,
        UnderTemperatureAlarm,
        ConspicuityDeviceFailure,
        SignLuminanceControllerFailure,
        ControllerReset,
        BatteryLow,
        PoweredOffByCommand,
        FacilitySwitchoverride,
        SignDisplayDriverFailure,
        SignFirmwareMismatch,
        SignLampPairFailure,
        EquipmentOverTemperature,
        NoResponseFromSensor,
        CutSensorCable,
        SensorShortCircuit,
        SensorDirtyLens,
        HARHardwareError,
        HARRadioFault,
        HARVoiceDataError,
        DisplayTimeoutError,
        BackupControllerUnavailable,
        PreexistingOrReoccurringFaultExists = 0xFF
    };
}
namespace MULTI
{
    enum COLOUR
    {
        OFF,
        Red,
        Yellow,
        Green,
        Cyan,
        Blue,
        Magenta,
        White,
        Orange,
        Amber,
    };
}

namespace FRM
{
    enum COLOUR
    {
        Default = 0,
        Red,
        Yellow,
        Green,
        Cyan,
        Blue,
        Magenta,
        White,
        Orange,
        Amber,
        MultipleColours = 0x0D,
        RGB24 = 0x0E,
    };
}

namespace ANNULUS
{
    enum CODE
    {
        Off = 0,
        Flashing = 0x08,
        On = 0x10
    };
}

namespace CONSP
{
    enum CODE
    {
        Off = 0,
        UpDown,
        LeftRight,
        WigWag,
        AllFlash,
        AllOn
    };
}

namespace SESR
{
    enum SIGN_TYPE
    {
        TEXT,
        GFX,
        ADVGFX
    };
}

namespace SCR
{
    enum SIGN_TYPE
    {
        TEXT,
        GFXMONO,
        GFXMULTI,
        GFXRGB24
    };
}

namespace LOG
{
    enum TYPE
    {
        FaultLog20 = 0,
        FaultLog500 = 0x0B,
        AlarmLog = 0x0C,
        EventLog = 0x0D
    };
}

/// \brief High-res gfx frame size
/// max size of a 24-bit RGB frame is 288*64*3 = 55296 bytes
/// total bytes : 15 + 55296 = 55311
#define MAX_HRGFRM_SIZE 55311

/// \brief Gfx frame size
// max size of a 4-bit frame is 256*64/2 = 8192 bytes
// total bytes : 11 + 8192 = 8203
#define MAX_GFXFRM_SIZE 8203

/// \brief Text frame size
// max size of a text frame is 255 bytes
// total bytes : 9 + 255 = 264
#define MAX_TXTFRM_SIZE 264

/// \brief App packet size
/// max size of app packet is HRG frame
#define MAX_APP_PACKET_SIZE MAX_HRGFRM_SIZE


/// \brief Data packet Header and End of Block size
#define DATA_PACKET_HEADER_SIZE    8
#define DATA_PACKET_EOB_SIZE    5

/// \brief Data packet size
/// max size of data packet is HRG frame at original format
/// note: There are 1000 event logs, assume the logs size is less than MAX_HRGFRM_SIZE
#define MAX_DATA_PACKET_SIZE (DATA_PACKET_HEADER_SIZE + MAX_HRGFRM_SIZE*2 + DATA_PACKET_EOB_SIZE)

/// \brief Data packet size
#define NON_DATA_PACKET_SIZE 10

/// \brief Packet acknowledgment(Non data packet) + Data packet
#define MAX_ACK_DATA_PACKET_SIZE (NON_DATA_PACKET_SIZE + MAX_DATA_PACKET_SIZE)

namespace CTRL_CHAR
{
    enum
    {
        SOH = 1,
        STX = 2,
        ETX = 3,
        EOT = 4,
        ACK = 6,
        NAK = 15
    };
}


#endif
