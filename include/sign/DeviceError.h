#pragma once

#include <tsisp003/TsiSp003Const.h>
#include <module/Utils.h>

class DeviceError
{
protected:
    const DEV::ERROR *devErr;
    int size;
    Utils::Bits errBit{32};

    // \return  true: state changed, false: state NOT changed
    bool DevError(DEV::ERROR code, bool v);

public:
    DeviceError(int size);
    virtual ~DeviceError();
    void Clear() { errBit.ClrAll(); };
    Utils::Bits & GetV() { return errBit; };
    void SetV(Utils::Bits & v) { errBit.Clone(v); } ;
    bool IsSet(DEV::ERROR code);

    // highest priority error code
    DEV::ERROR GetErrorCode();
};

class SignError : public DeviceError
{
private:
    static const DEV::ERROR SIGNERROR[];

public:
    SignError();
    ~SignError();
    bool Push(uint8_t sid, DEV::ERROR code, bool v);
};

class CtrllerError : public DeviceError
{
private:
    static const DEV::ERROR CTRLERROR[];

public:
    CtrllerError();
    ~CtrllerError();

    // \return  true: state changed, false: state NOT changed
    bool Push(DEV::ERROR code, bool v);
};
