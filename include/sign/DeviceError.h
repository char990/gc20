#pragma once

#include <tsisp003/TsiSp003Const.h>
#include <module/Utils.h>

class DeviceError
{
protected:
    const DEV::ERROR *devErr;
    int size;
    Utils::Bool32 errBit{0};

    // \return  true: state changed, false: state NOT changed
    bool DevError(DEV::ERROR code, bool v);

public:
    DeviceError(int size);
    virtual ~DeviceError();
    void Reset() { errBit.Set(0); };
    uint32_t GetV() { return errBit.Get(); };
    void SetV(uint32_t v) { errBit.Set(v); } ;

    // highest priority error code
    DEV::ERROR GetErrorCode();
};

class SignError : public DeviceError
{
private:
    static const DEV::ERROR signErr[9];

public:
    SignError();
    ~SignError();
    bool Push(uint8_t sid, DEV::ERROR code, bool v);
};

class CtrllerError : public DeviceError
{
private:
    static const DEV::ERROR ctrllerErr[9];

public:
    CtrllerError();
    ~CtrllerError();

    // \return  true: state changed, false: state NOT changed
    bool Push(DEV::ERROR code, bool v);
};
