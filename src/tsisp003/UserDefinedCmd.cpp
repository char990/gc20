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
    case 0x0A:
        FA0A_RetrieveLogs(data, len);
        break;
    case 0x0F:
        FA0F_ResetLogs(data, len);
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
    case 0x0B:
    {
        auto &log = db.GetUciFault();
        applen = log.GetLog(txbuf + 2);
    }
    break;
    case 0x0C:
    {
        auto &log = db.GetUciAlarm();
        applen = log.GetLog(txbuf + 2);
    }
    break;
    case 0x0D:
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
    Tx(txbuf, applen+2);
    return 0;
}

int TsiSp003App::FA0F_ResetLogs(uint8_t *data, int len)
{
    auto &db = DbHelper::Instance();
    uint8_t subcmd = *(data + 2);
    switch (subcmd)
    {
    case 0x01:
    {
        auto &log = db.GetUciFault();
        log.Reset();
        db.GetUciEvent().Push(0, "ResetFaultLog");
    }
    break;
    case 0x02:
    {
        auto &log = db.GetUciAlarm();
        log.Reset();
        db.GetUciEvent().Push(0, "ResetAlarmLog");
    }
    break;
    case 0x03:
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
