#include <sign/DeviceError.h>
#include <module/MyDbg.h>
#include <uci/DbHelper.h>

/***************** DeviceError *****************/
DeviceError::DeviceError(int size)
    : size(size)
{
}

DeviceError::~DeviceError()
{
}

bool DeviceError::DevError(DEV::ERROR code, bool v)
{
    int i;
    for (i = 0; i < size; i++)
    {
        if (devErr[i] == code)
        {
            break;
        }
    }
    if (i == size)
    {
        MyThrow("Incorrect DEV::ERROR 0x%02X", code);
    }
    if (v != errBit.GetBit(i))
    {
        v ? errBit.SetBit(i) : errBit.ClrBit(i);
        return true;
    }
    return false;
}

DEV::ERROR DeviceError::GetErrorCode()
{
    int i = size;
    for (int k = 0; k < size; k++)
    {
        if (errBit.GetBit(i))
        {
            i = k;
        }
    }
    return (i == size) ? DEV::ERROR::DevNoError : devErr[i];
}

/***************** SignError *****************/
const DEV::ERROR SignError::signErr[9] = {
    DEV::ERROR::PoweredOffByCommand,
    DEV::ERROR::SignSingleLedFailure,
    DEV::ERROR::SignLuminanceControllerFailure,
    DEV::ERROR::ConspicuityDeviceFailure,
    DEV::ERROR::InternalPowerSupplyFault,
    DEV::ERROR::OverTemperatureAlarm,
    DEV::ERROR::SignMultiLedFailure,
    DEV::ERROR::SignDisplayDriverFailure,
    DEV::ERROR::InternalCommunicationsFailure};

SignError::SignError()
    : DeviceError(10)
{
    devErr = signErr;
}

SignError::~SignError()
{
}

bool SignError::Push(uint8_t sid, DEV::ERROR code, bool v)
{
    if (DevError(code, v))
    {
        DbHelper::Instance().GetUciFault().Push(sid, code, v ? 1 : 0);
        DbHelper::Instance().GetUciProcess().SaveSignErr(sid, errBit.Get());
        return true;
    }
    return false;
}

/***************** CtrllerError *****************/
const DEV::ERROR CtrllerError::ctrllerErr[9] = {
    DEV::ERROR::CommunicationsTimeoutError,
    DEV::ERROR::DisplayTimeoutError,
    DEV::ERROR::EquipmentOverTemperature,
    DEV::ERROR::ControllerResetViaWatchdog,
    DEV::ERROR::BatteryLow,
    DEV::ERROR::BatteryFailure,
    DEV::ERROR::FacilitySwitchOverride,
    DEV::ERROR::MemoryError,
    DEV::ERROR::PowerFailure};

CtrllerError::CtrllerError()
    : DeviceError(10)
{
    devErr = ctrllerErr;
}

CtrllerError::~CtrllerError()
{
}

bool CtrllerError::Push(DEV::ERROR code, bool v)
{
    if (DeviceError::DevError(code, v))
    {
        DbHelper::Instance().GetUciFault().Push(0, code, v ? 1 : 0);
        DbHelper::Instance().GetUciProcess().SaveCtrllerErr(errBit.Get());
        return true;
    }
    return false;
}
