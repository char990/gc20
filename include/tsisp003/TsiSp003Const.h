#pragma once

#include <cstdint>
#include <vector>

class MI
{
public:
    enum class CODE : uint8_t
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

    FACMD_SIGNTEST = 0x30,

    FACMD_SHAKE_RQST = 0xF0,
    FACMD_SHAKE_REPLY = 0xF1,
    FACMD_SHAKE_PASSWD = 0xF2,
    FACMD_RESTART = 0xF5,
    FACMD_REBOOT = 0xFA
};

class APP
{
public:
    enum class ERROR : uint8_t
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
        UserDefinedFE = 0xFE,
        UNDEFINED = 0xFF
    };

    struct sAppErrorStr
    {
        ERROR code;
        const char *str;
    };

    static std::vector<sAppErrorStr> apperror_str;
    static const char *ToStr(uint8_t);
    static const char *ToStr(ERROR);
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

//#define HALF_BYTE
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
    Multi_4bit = 0x0D,
    RGB_24bit = 0x0E,
#ifdef HALF_BYTE
    HalfByte = 0xF1, // Brightway Master-Slave poctocol V2.2
#endif
    UNDEFINED = 0xFF
};

#define GetAnnulus(b) ((b>>3)&0x03)
#define GetConspicuity(b) (b&0x07)

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
/// max size of app packet is EventLog (1000 entries of event log)
#define MAX_APP_PACKET_SIZE (4 + (1 + 2 + 7 + 65) * 1000)

/// \brief Data packet Header and End of Block size
#define DATA_PACKET_HEADER_SIZE 8
#define DATA_PACKET_EOB_SIZE 5

/// \brief Data packet size
#define MAX_DATA_PACKET_SIZE (DATA_PACKET_HEADER_SIZE + MAX_APP_PACKET_SIZE * 2 + DATA_PACKET_EOB_SIZE)
#define POWEROF2_MAX_DATA_PACKET_SIZE (2 * 65536)

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
    SOH = 0x01,
    STX = 0x02,
    ETX = 0x03,
    EOT = 0x04,
    ACK = 0x06,
    NAK = 0x15,
    UNDEFINED = 0xFF
};

#define TSISP003VER_SIZE 3
extern const char *TSISP003VER[TSISP003VER_SIZE];

#define PRODTYPE_SIZE 2
extern const char *PRODTYPE[PRODTYPE_SIZE];
enum class PRODUCT : uint8_t
{
    VMS,
    ISLUS
};

#define WEEKDAY_SIZE 7
extern const char *WEEKDAY[WEEKDAY_SIZE];

#define TEST_PIXELS_SIZE 5
extern const char *TestPixels[TEST_PIXELS_SIZE];

#define ANNULUS_SIZE 3
extern const char *Annulus[ANNULUS_SIZE];

#define CONSPICUITY_SIZE 6
extern const char *Conspicuity[CONSPICUITY_SIZE];

class FrameColour
{
public:
#define MONO_COLOUR_NAME_SIZE 10
#define ALL_COLOUR_NAME_SIZE 12

    static const char *COLOUR_NAME[ALL_COLOUR_NAME_SIZE];
    static const int COLOUR_RGB8[MONO_COLOUR_NAME_SIZE];
    static int GetRGB8(uint8_t mappedc);
    static const uint8_t COLOUR_RGB1[MONO_COLOUR_NAME_SIZE];
    static uint8_t GetRGB1(uint8_t mappedc);
    // convert int=rgb to colour code(NOT mapped)
    static uint8_t Rgb2Colour(int32_t);
    static const char *GetColourName(uint8_t code);
};

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
