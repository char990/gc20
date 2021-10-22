#include <tsisp003/TsiSp003Const.h>

const char *TSISP003VER[TSISP003VER_SIZE] = {
    "QLD,2.1",
    "NSW,3.1",
    "NSW,5.0"};

const char *PRODTYPE[PRODTYPE_SIZE] = {
    "VMS",   // 1 group of VMS at 1 com, 1 group has 1 sign, 1 sign has 1-x slaves, sign id is not equal to slave id
    "ISLUS", //  1 group of ISLUS at 1 com, 1 group has 1-x sign, 1 sign has 1 slave, slave id is same as sign Id
};

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

const char *DEV::GetStr(DEV::ERROR err)
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
        return STR[err];
    }
};
