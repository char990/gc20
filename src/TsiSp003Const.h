#ifndef __TSISP003CONST_H__
#define __TSISP003CONST_H__

namespace MI_CODE
{
    enum
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
namespace APP_ERROR
{
    enum
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

namespace DEV_ERROR
{
    enum
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

namespace ANNULUS
{
    enum
    {
        Off = 0,
        Flashing = 0x08,
        On = 0x10
    }
}

namespace CONSP
{
    enum
    {
        Off = 0,
        UpDown,
        LeftRight,
        WigWag,
        AllFlash,
        AllOn
    }
}

namespace MULTI_COLOURS
{
    enum
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

namespace FRM_COLOUR
{
    enum
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

namespace SESR_SIGN_TYPE
{
    enum
    {
        TEXT,
        GFX,
        ADVGFX
    };
}

namespace SCR_SIGN_TYPE
{
    enum
    {
        TEXT,
        GFXMONO,
        GFXMULTI,
        GFXRGB24
    };
}

namespace FRM_TYPE
{
    enum
    {
        TXT=0x0A,
        GFX=0x0B,
        HRG=0x1D
    };
}

namespace LOG_TYPE
{
    enum
    {
        FaultLog20 = 0,
        FaultLog500 = 0x0B,
        AlarmLog = 0x0C,
        EventLog = 0x0D
    };
}

#endif
