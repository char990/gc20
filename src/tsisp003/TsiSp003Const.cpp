#include <tsisp003/TsiSp003Const.h>
#include <vector>

const char *TSISP003VER[TSISP003VER_SIZE] = {
    "QLD,2.1",
    "NSW,3.1",
    "NSW,5.0"};

const char *PRODTYPE[PRODTYPE_SIZE] = {
    "VMS",   // 1 group of VMS at 1 com, 1 group has 1 sign, 1 sign has 1-x slaves, sign id is not equal to slave id
    "ISLUS", //  1 group of ISLUS at 1 com, 1 group has 1-x sign, 1 sign has 1 slave, slave id is same as sign Id
};

// index is coulur code
const char *FrameColour::COLOUR_NAME[COLOUR_NAME_SIZE] = {
    "DEFAULT",
    "RED",
    "YELLOW",
    "GREEN",
    "CYAN",
    "BLUE",
    "MAGENTA",
    "WHITE",
    "ORANGE",
    "AMBER"};

const int FrameColour::COLOUR_RGB8[COLOUR_NAME_SIZE] = {
    0x000000, // BLACK
    0xFF0000, // Red
    0xFFFF00, // Yellow
    0x00FF00, // Green
    0x00FFFF, // Cyan
    0x0000FF, // Blue
    0xFF00FF, // Magenta
    0xFFFFFF, // White
    0xFFA500, // Orange
    0xFFBF00  // Amber
};

const uint8_t FrameColour::COLOUR_RGB1[COLOUR_NAME_SIZE] = {
    // only get bit-7 of colour value,so there are only 0-7
    0x00, // 0x000000, // Black
    0x04, // 0xFF0000, // Red
    0x06, // 0xFFFF00, // Yellow
    0x02, // 0x00FF00, // Green
    0x03, // 0x00FFFF, // Cyan
    0x01, // 0x0000FF, // Blue
    0x05, // 0xFF00FF, // Magenta
    0x07, // 0xFFFFFF, // White
    0x06, // 0xFFA500, // Orange
    0x06, // 0xFFBF00  // Amber
};

uint8_t FrameColour::Rgb2Colour(int32_t rgb)
{
    uint8_t c=0;
    if(rgb&0x000080)
    {
        c|=1;
    }
    if(rgb&0x008000)
    {
        c|=2;
    }
    if(rgb&0x800000)
    {
        c|=4;
    }
    for(int i=0;i<COLOUR_NAME_SIZE;i++)
    {
        if(c==COLOUR_RGB1[i])
        {
            return i;
        }
    }
    return 0;
}

std::vector<MI::sMiCodeStr> MI::micode_str{
    {MI::CODE::Reject, "Reject"},
    {MI::CODE::Ack, "Ack"},
    {MI::CODE::StartSession, "StartSession"},
    {MI::CODE::PasswordSeed, "PasswordSeed"},
    {MI::CODE::Password, "Password"},
    {MI::CODE::HeartbeatPoll, "HeartbeatPoll"},
    {MI::CODE::SignStatusReply, "SignStatusReply"},
    {MI::CODE::EndSession, "EndSession"},
    {MI::CODE::SystemReset, "SystemReset"},
    {MI::CODE::UpdateTime, "UpdateTime"},
    {MI::CODE::SignSetTextFrame, "SignSetTextFrame"},
    {MI::CODE::SignSetGraphicsFrame, "SignSetGraphicsFrame"},
    {MI::CODE::SignSetMessage, "SignSetMessage"},
    {MI::CODE::SignSetPlan, "SignSetPlan"},
    {MI::CODE::SignDisplayFrame, "SignDisplayFrame"},
    {MI::CODE::SignDisplayMessage, "SignDisplayMessage"},
    {MI::CODE::EnablePlan, "EnablePlan"},
    {MI::CODE::DisablePlan, "DisablePlan"},
    {MI::CODE::RequestEnabledPlans, "RequestEnabledPlans"},
    {MI::CODE::ReportEnabledPlans, "ReportEnabledPlans"},
    {MI::CODE::SignSetDimmingLevel, "SignSetDimmingLevel"},
    {MI::CODE::PowerOnOff, "PowerOnOff"},
    {MI::CODE::DisableEnableDevice, "DisableEnableDevice"},
    {MI::CODE::SignRequestStoredFMP, "SignRequestStoredFMP"},
    {MI::CODE::RetrieveFaultLog, "RetrieveFaultLog"},
    {MI::CODE::FaultLogReply, "FaultLogReply"},
    {MI::CODE::ResetFaultLog, "ResetFaultLog"},
    {MI::CODE::SignExtendedStatusRequest, "SignExtendedStatusRequest"},
    {MI::CODE::SignExtendedStatusReply, "SignExtendedStatusReply"},
    {MI::CODE::SignSetHighResolutionGraphicsFrame, "SignSetHighResolutionGraphicsFrame"},
    {MI::CODE::SignConfigurationRequest, "SignConfigurationRequest"},
    {MI::CODE::SignConfigurationReply, "SignConfigurationReply"},
    {MI::CODE::SignDisplayAtomicFrames, "SignDisplayAtomicFrames"},
    {MI::CODE::HARStatusReply, "HARStatusReply"},
    {MI::CODE::HARSetVoiceDataIncomplete, "HARSetVoiceDataIncomplete"},
    {MI::CODE::HARSetVoiceDataComplete, "HARSetVoiceDataComplete"},
    {MI::CODE::HARSetStrategy, "HARSetStrategy"},
    {MI::CODE::HARActivateStrategy, "HARActivateStrategy"},
    {MI::CODE::HARSetPlan, "HARSetPlan"},
    {MI::CODE::HARRequestStoredVSP, "HARRequestStoredVSP"},
    {MI::CODE::HARSetVoiceDataACK, "HARSetVoiceDataACK"},
    {MI::CODE::HARSetVoiceDataNAK, "HARSetVoiceDataNAK"},
    {MI::CODE::EnvironmentalWeatherStatusReply, "EnvironmentalWeatherStatusReply"},
    {MI::CODE::RequestEnvironmentalWeatherValues, "RequestEnvironmentalWeatherValues"},
    {MI::CODE::EnvironmentalWeatherValues, "EnvironmentalWeatherValues"},
    {MI::CODE::EnvironmentalWeatherThresholdDefinition, "EnvironmentalWeatherThresholdDefinition"},
    {MI::CODE::RequestThresholdDefinition, "RequestThresholdDefinition"},
    {MI::CODE::RequestEnvironmentalWeatherEventLog, "RequestEnvironmentalWeatherEventLog"},
    {MI::CODE::EnvironmentalWeatherEventLogReply, "EnvironmentalWeatherEventLogReply"},
    {MI::CODE::ResetEnvironmentalWeatherEventLog, "ResetEnvironmentalWeatherEventLog"},
    {MI::CODE::UserDefinedCmdF0, "UserDefinedCmdF0"},
    {MI::CODE::UserDefinedCmdF1, "UserDefinedCmdF1"},
    {MI::CODE::UserDefinedCmdF2, "UserDefinedCmdF2"},
    {MI::CODE::UserDefinedCmdF3, "UserDefinedCmdF3"},
    {MI::CODE::UserDefinedCmdF4, "UserDefinedCmdF4"},
    {MI::CODE::UserDefinedCmdF5, "UserDefinedCmdF5"},
    {MI::CODE::UserDefinedCmdF6, "UserDefinedCmdF6"},
    {MI::CODE::UserDefinedCmdF7, "UserDefinedCmdF7"},
    {MI::CODE::UserDefinedCmdF8, "UserDefinedCmdF8"},
    {MI::CODE::UserDefinedCmdF9, "UserDefinedCmdF9"},
    {MI::CODE::UserDefinedCmdFA, "UserDefinedCmdFA"},
    {MI::CODE::UserDefinedCmdFB, "UserDefinedCmdFB"},
    {MI::CODE::UserDefinedCmdFC, "UserDefinedCmdFC"},
    {MI::CODE::UserDefinedCmdFD, "UserDefinedCmdFD"},
    {MI::CODE::UserDefinedCmdFE, "UserDefinedCmdFE"},
    {MI::CODE::UserDefinedCmdFF, "UserDefinedCmdFF"}};

const char *MI::ToStr(uint8_t code)
{
    for (auto &s : micode_str)
    {
        if (code == static_cast<uint8_t>(s.code))
        {
            return s.str;
        }
    }
    return "Unknown Mi code";
}

std::vector<APP::sAppErrorStr> APP::apperror_str{
    {APP::ERROR::AppNoError, "AppNoError"},
    {APP::ERROR::DeviceControllerOffline, "DeviceControllerOffline"},
    {APP::ERROR::SyntaxError, "SyntaxError"},
    {APP::ERROR::LengthError, "LengthError"},
    {APP::ERROR::DataChksumError, "DataChksumError"},
    {APP::ERROR::TextNonASC, "TextNonASC"},
    {APP::ERROR::FrameTooLarge, "FrameTooLarge"},
    {APP::ERROR::UnknownMi, "UnknownMi"},
    {APP::ERROR::MiNotSupported, "MiNotSupported"},
    {APP::ERROR::PowerIsOff, "PowerIsOff"},
    {APP::ERROR::UndefinedDeviceNumber, "UndefinedDeviceNumber"},
    {APP::ERROR::FontNotSupported, "FontNotSupported"},
    {APP::ERROR::ColourNotSupported, "ColourNotSupported"},
    {APP::ERROR::OverlaysNotSupported, "OverlaysNotSupported"},
    {APP::ERROR::DimmingLevelNotSupported, "DimmingLevelNotSupported"},
    {APP::ERROR::FrmMsgPlnActive, "FrmMsgPlnActive"},
    {APP::ERROR::FacilitySwitchOverride, "FacilitySwitchOverride"},
    {APP::ERROR::ConspicuityNotSupported, "ConspicuityNotSupported"},
    {APP::ERROR::TransitionTimeNotSupported, "TransitionTimeNotSupported"},
    {APP::ERROR::FrmMsgPlnUndefined, "FrmMsgPlnUndefined"},
    {APP::ERROR::PlanNotEnabled, "PlanNotEnabled"},
    {APP::ERROR::PlanEnabled, "PlanEnabled"},
    {APP::ERROR::SizeMismatch, "SizeMismatch"},
    {APP::ERROR::FrameTooSmall, "FrameTooSmall"},
    {APP::ERROR::HARStrategyStopped, "HARStrategyStopped"},
    {APP::ERROR::HarStrategyUndefined, "HarStrategyUndefined"},
    {APP::ERROR::HARStrategyError, "HARStrategyError"},
    {APP::ERROR::HARVoiceError, "HARVoiceError"},
    {APP::ERROR::HARVoiceNotSupported, "HARVoiceNotSupported"},
    {APP::ERROR::HARHardwareError, "HARHardwareError"},
    {APP::ERROR::TimeExpired, "TimeExpired"},
    {APP::ERROR::CollourDepthNotSupported, "CollourDepthNotSupported"},
    {APP::ERROR::IncompleteColouFrameDefinition, "IncompleteColouFrameDefinition"},
    {APP::ERROR::IncorrectPassword, "IncorrectPassword"},
    {APP::ERROR::InterlockingInvalidSetting, "InterlockingInvalidSetting"},
    {APP::ERROR::InterlockingMissingSigns, "InterlockingMissingSigns"},
    {APP::ERROR::InterlockingNotActive, "InterlockingNotActive"},
    {APP::ERROR::InterlocckingActive, "InterlocckingActive"},
    {APP::ERROR::UserDefinedFE, "UserDefinedFE"},
    {APP::ERROR::UNDEFINED, "UNDEFINED"},
};

const char *APP::ToStr(uint8_t code)
{
    for (auto &s : apperror_str)
    {
        if (code == static_cast<uint8_t>(s.code))
        {
            return s.str;
        }
    }
    return "Unknown App Error";
}

const char *DEV::STR[] = {
    "DevNoError",
    "PowerFailure",
    "CommunicationsTimeoutError",
    "MemoryError",
    "BatteryFailure",
    "InternalCommunicationsFailure",
    "SignLampFailure",
    "SignSingleLedFailure",
    "SignMultiLedFailure",
    "OverTemperatureAlarm",
    "UnderTemperatureAlarm",
    "ConspicuityDeviceFailure",
    "SignLuminanceControllerFailure",
    "ControllerResetViaWatchdog",
    "BatteryLow",
    "PoweredOffByCommand",
    "FacilitySwitchOverride",
    "SignDisplayDriverFailure",
    "SignFirmwareMismatch",
    "SignLampPairFailure",
    "EquipmentOverTemperature",
    "NoResponseFromSensor",
    "CutSensorCable",
    "SensorShortCircuit",
    "SensorDirtyLens",
    "HARHardwareError",
    "HARRadioFault",
    "HARVoiceDataError",
    "DisplayTimeoutError",
    "BackupControllerUnavailable",
    "NotAllocated1E",
    "NotAllocated1F",
    "UnderLocalControl",
    "MainProcessorCommunicationsError",
    "MimicStateError",
    "SignMovedFromSetLocation",
    "CabinetDoorOpen",
    "SignTilted",
    "SignorientationChanged",
    "BatteryChargerRegulatorFault",
    "InternalPowerSupplyFault",
    "VibrationAlarm",
    "OperatingOnSecondaryPower"};

const char *DEV::ToStr(DEV::ERROR err)
{
    if (err == ERROR::PreexistingOrReoccurringFaultExists)
    {
        return "PreexistingOrReoccurringFaultExists";
    }
    else if (err > ERROR::OperatingOnSecondaryPower)
    {
        return ("Undefined Controller/Device Error code");
    }
    else
    {
        return STR[static_cast<uint8_t>(err)];
    }
};
