#pragma once

#include <cstdint>
#include <vector>

class MI
{
public:
    enum class CODE: uint8_t
    {
        Reject = 0x00,
        Ack,
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
        PowerOnOff,
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

    struct sMiCodeStr
    {
        CODE code;
        const char *str;
    };

    static std::vector<sMiCodeStr> micode_str;

    static const char *ToStr(uint8_t);
};

class APP
{
public:
    enum class ERROR: uint8_t
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
        IncorrectPassword,
        InterlockingInvalidSetting,
        InterlockingMissingSigns,
        InterlockingNotActive,
        InterlocckingActive,
        UNDEFINED = 0xFF
    };

    struct sAppErrorStr
    {
        ERROR code;
        const char *str;
    };

    static std::vector<sAppErrorStr> apperror_str;
    static const char *ToStr(uint8_t);
};

class DEV
{
public:
    enum class ERROR : uint8_t
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
        ControllerResetViaWatchdog,
        BatteryLow,
        PoweredOffByCommand,
        FacilitySwitchOverride,
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
        NotAllocated1E,
        NotAllocated1F,
        UnderLocalControl,
        MainProcessorCommunicationsError,
        MimicStateError,
        SignMovedFromSetLocation,
        CabinetDoorOpen,
        SignTilted,
        SignOrientationChanged,
        BatteryChargerRegulatorFault,
        InternalPowerSupplyFault,
        VibrationAlarm,
        OperatingOnSecondaryPower,
        PreexistingOrReoccurringFaultExists = 0xFF
    };
    static const char *STR[];
    static const char *ToStr(enum ERROR err);
};

enum class MULTICOLOUR : uint8_t
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
    UNDEFINED = 0xFF
};

enum class FRMCOLOUR : uint8_t
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
    MonoFinished,
    MultipleColours = 0x0D,
    RGB24 = 0x0E,
    UNDEFINED = 0xFF
};

enum class ANNULUS : uint8_t
{
    Off = 0,
    Flashing = 0x08,
    On = 0x10,
    UNDEFINED = 0xFF
};

enum class CONSPICUITY : uint8_t
{
    Off = 0,
    UpDown,
    LeftRight,
    WigWag,
    AllFlash,
    AllOn,
    UNDEFINED = 0xFF
};

enum class SESR_SIGN_TYPE : uint8_t
{
    TEXT,
    GFX,
    ADVGFX,
    UNDEFINED = 0xFF
};

enum class SCR_SIGN_TYPE : uint8_t
{
    TEXT,
    GFXMONO,
    GFXMULTI,
    GFXRGB24,
    UNDEFINED = 0xFF
};

enum class LOG_TYPE : uint8_t
{
    FaultLog20 = 0,
    FaultLog500 = 0x0B,
    AlarmLog = 0x0C,
    EventLog = 0x0D,
    UNDEFINED = 0xFF
};

enum class DISP_TYPE : uint8_t
{
    N_A, // not available
    FRM, // frame (0 should to be convert to plan)
    MSG, // message (0 should to be convert to plan), not including ExtSw
    PLN, // plan (get from frm0/msg0)
    ATF, // atomic frame
    EXT, // external input
    FSW, // facility switch
    BLK, // Blank
    UNDEFINED = 0xFF
};

/// \brief High-res gfx frame size
/// max size of a 24-bit RGB frame is 288*64*3 = 55296 bytes
/// total bytes : 13 + 55296 + 2 = 55311
#define HRGFRM_HEADER_SIZE 13
//#define MAX_HRGFRM_SIZE (HRGFRM_HEADER_SIZE + 288*64*3 + 2)
//#define MIN_HRGFRM_SIZE (HRGFRM_HEADER_SIZE + 1 + 2)

/// \brief Gfx frame size
// max size of a 4-bit frame is 255*64/2 = 8160 bytes
// total bytes : 9 + 8192 +2 = 8171
#define GFXFRM_HEADER_SIZE 9
//#define MAX_GFXFRM_SIZE (GFXFRM_HEADER_SIZE + 255*64/2 + 2)
//#define MIN_GFXFRM_SIZE (GFXFRM_HEADER_SIZE + 1 + 2)

/// \brief Text frame size
// max size of a text frame is 255 bytes
// total bytes : 7 + 255 + 2 = 264
#define TXTFRM_HEADER_SIZE 7
//#define MAX_TXTFRM_SIZE (TXTFRM_HEADER_SIZE + 255 + 2)
//#define MIN_TXTFRM_SIZE (TXTFRM_HEADER_SIZE + 1 + 2)

/// \brief App packet size
/// max size of app packet is Send File Packet ( >1000 entries of event log)
#define MAX_APP_PACKET_SIZE (64 * 1024 + 4)

/// \brief Data packet Header and End of Block size
#define DATA_PACKET_HEADER_SIZE 8
#define DATA_PACKET_EOB_SIZE 5

/// \brief Data packet size
#define MAX_DATA_PACKET_SIZE (DATA_PACKET_HEADER_SIZE + MAX_APP_PACKET_SIZE * 2 + DATA_PACKET_EOB_SIZE)

/// \brief Data packet size
#define NON_DATA_PACKET_SIZE 10

/// \brief Packet acknowledgment(Non data packet) + Data packet
#define MAX_ACK_DATA_PACKET_SIZE (NON_DATA_PACKET_SIZE + MAX_DATA_PACKET_SIZE)

#define MAX_FONT 5
#define MAX_MONOCOLOUR 9

#define OFFSET_MI_CDOE 0

#define OFFSET_FRM_ID (OFFSET_MI_CDOE + 1)
#define OFFSET_FRM_REV (OFFSET_FRM_ID + 1)

#define OFFSET_MSG_ID (OFFSET_MI_CDOE + 1)
#define OFFSET_MSG_REV (OFFSET_MSG_ID + 1)

#define OFFSET_PLN_ID (OFFSET_MI_CDOE + 1)
#define OFFSET_PLN_REV (OFFSET_PLN_ID + 1)

enum class CTRL_CHAR : uint8_t
{
    SOH = 1,
    STX = 2,
    ETX = 3,
    EOT = 4,
    ACK = 6,
    NAK = 15,
    UNDEFINED = 0xFF
};

#define TSISP003VER_SIZE 3
extern const char *TSISP003VER[TSISP003VER_SIZE];

#define PRODTYPE_SIZE 2
extern const char *PRODTYPE[PRODTYPE_SIZE];

enum class FCLTSWITCH : uint8_t
{
    FS_AUTO,
    FS_MSG1,
    FS_MSG2,
    FS_OFF = 255
};
enum class EXTSWITCH : uint8_t
{
    EXT_NONE,
    EXT_SW3,
    EXT_SW4,
    EXT_SW5,
    UNDEFINED = 0xFF
};
enum class DISPSTATE : uint8_t
{
    DISP_NONE,
    DISP_FRM,
    DISP_MSG,
    DISP_PLN,
    DISP_FCLTSW,
    DISP_EXTSW,
    DISP_PWR_OFF,
    DISP_DISABLED,
    UNDEFINED = 0xFF
};

#define CTRLLER_TICK 10 // ms

#define CTRLLER_MS(x) (x / CTRLLER_TICK)
