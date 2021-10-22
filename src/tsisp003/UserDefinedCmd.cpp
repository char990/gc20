#include <tsisp003/TsiSp003App.h>

int TsiSp003App::UserDefinedCmd(uint8_t *data, int len)
{
    switch (*data)
    {
    case MI::CODE::UserDefinedCmdFA:
        UserDefinedCmdFA(data, len);
        break;
    default:
        return -1;
    }
    return 0;
}

int TsiSp003App::UserDefinedCmdFA(uint8_t *data, int len)
{
    switch (*(data + 1))
    {
    case FACMD_RTRV_LOGS:
        FA0A_RetrieveLogs(data, len);
        break;
    case FACMD_RESET_LOGS:
        FA0F_ResetLogs(data, len);
        break;
    case FACMD_RQST_USER_EXT:
        FA22_RqstUserExt(data, len);
        break;
    default:
        Reject(APP::ERROR::SyntaxError);
    }
    return 0;
}

int TsiSp003App::FA0A_RetrieveLogs(uint8_t *data, int len)
{
    auto &db = DbHelper::Instance();
    int applen;
    uint8_t subcmd = *(data + 2);
    switch (subcmd)
    {
    case FACMD_RPL_FLT_LOGS:
    {
        auto &log = db.GetUciFault();
        applen = log.GetLog(txbuf + 2);
    }
    break;
    case FACMD_RPL_ALM_LOGS:
    {
        auto &log = db.GetUciAlarm();
        applen = log.GetLog(txbuf + 2);
    }
    break;
    case FACMD_RPL_EVT_LOGS:
    {
        auto &log = db.GetUciEvent();
        applen = log.GetLog(txbuf + 2);
    }
    break;
    default:
        Reject(APP::ERROR::SyntaxError);
        return 0;
    }
    txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
    txbuf[1] = subcmd;
    Tx(txbuf, applen + 2);
    return 0;
}

int TsiSp003App::FA0F_ResetLogs(uint8_t *data, int len)
{
    auto &db = DbHelper::Instance();
    uint8_t subcmd = *(data + 2);
    switch (subcmd)
    {
    case FACMD_RPL_FLT_LOGS:
    {
        auto &log = db.GetUciFault();
        log.Reset();
        db.GetUciEvent().Push(0, "ResetFaultLog");
    }
    break;
    case FACMD_RPL_ALM_LOGS:
    {
        auto &log = db.GetUciAlarm();
        log.Reset();
        db.GetUciEvent().Push(0, "ResetAlarmLog");
    }
    break;
    case FACMD_RPL_EVT_LOGS:
    {
        auto &log = db.GetUciEvent();
        log.Reset();
        db.GetUciEvent().Push(0, "ResetEventLog");
    }
    break;
    default:
        Reject(APP::ERROR::SyntaxError);
        return 0;
    }
    Ack();
    return 0;
}

extern const char *FirmwareMajorVer;
extern const char *FirmwareMinorVer;

int TsiSp003App::FA22_RqstUserExt(uint8_t *data, int len)
{
    txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
    txbuf[1] = FACMD_RPL_USER_EXT;
	auto pt = txbuf+2;
    auto & ctrl = Controller::Instance();
    auto sign = ctrl.GetGroup(1)->GetSign(1);
	*pt++ = sign->MaxTemp();
	*pt++ = sign->CurTemp();
	*pt++ = ctrl.MaxTemp();
	*pt++ = ctrl.CurTemp();
    pt=Utils::Cnvt::PutU16(sign->Voltage(), pt);
    pt=Utils::Cnvt::PutU16(sign->Lux(), pt);
	char * v = DbHelper::Instance().GetUciProd().MfcCode()+4;
	*pt++ = *v; // Get PCB revision from MANUFACTURER_CODE
	*pt++ = *(v+1); // Get Sign type from MANUFACTURER_CODE
	*pt++ = *FirmwareMajorVer;
	*pt++ = *(FirmwareMajorVer+1);
	*pt++ = *FirmwareMinorVer;
	*pt++ = *(FirmwareMinorVer+1);
    Tx(txbuf, pt-txbuf);
    return 0;
}
