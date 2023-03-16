#include <cstdlib>
#include <openssl/md5.h>
#include <tsisp003/TsiSp003App.h>

extern const char *FirmwareVer;

using namespace Utils;

int TsiSp003App::UserDefinedCmd(uint8_t *data, int len)
{
    if (CheckOnline_RejectIfFalse())
    {
        auto mi = static_cast<MI::CODE>(*data);
        switch (mi)
        {
        case MI::CODE::UserDefinedCmdFA:
            UserDefinedCmdFA(data, len);
            break;
        case MI::CODE::UserDefinedCmdFE:
            FE_SetGuiConfig(data, len);
            break;
        case MI::CODE::UserDefinedCmdFF:
            FF_RqstGuiConfig(data, len);
            break;
        default:
            return -1;
        }
    }
    return 0;
}

int TsiSp003App::UserDefinedCmdFA(uint8_t *data, int len)
{
    switch (*(data + 1))
    {
    case FACMD_SET_LUMINANCE:
        FA01_SetLuminance(data, len);
        break;
    case FACMD_SET_EXT_INPUT:
        FA02_SetExtInput(data, len);
        break;
    case FACMD_RQST_EXT_INPUT:
        FA03_RqstExtInput(data, len);
        break;
    case FACMD_RQST_LUMINANCE:
        FA04_RqstLuminance(data, len);
        break;
    case FACMD_RTRV_LOGS:
        FA0A_RetrieveLogs(data, len);
        break;
    case FACMD_RESET_LOGS:
        FA0F_ResetLogs(data, len);
        break;
    case FACMD_SEND_FINFO:
        FA10_SendFileInfo(data, len);
        break;
    case FACMD_SEND_FPKT:
        FA11_SendFilePacket(data, len);
        break;
    case FACMD_START_UPGRD:
        FA12_StartUpgrading(data, len);
        break;
    case FACMD_SET_USER_CFG:
        FA20_SetUserCfg(data, len);
        break;
    case FACMD_RQST_USER_CFG:
        FA21_RqstUserCfg(data, len);
        break;
    case FACMD_RQST_USER_EXT:
        FA22_RqstUserExt(data, len);
        break;
    case FACMD_SHAKE_RQST:
        FAF0_ShakehandsRqst(data, len);
        break;
    case FACMD_SHAKE_PASSWD:
        FAF2_ShakehandsPasswd(data, len);
        break;
    case FACMD_RESTART:
        FAF5_Restart(data, len);
        break;
    case FACMD_REBOOT:
        FAFA_Reboot(data, len);
        break;
    case FACMD_SIGNTEST:
        FA30_SignTest(data, len);
        break;
    default:
        Reject(APP::ERROR::UnknownMi);
    }
    return 0;
}

int TsiSp003App::FA0A_RetrieveLogs(uint8_t *data, int len)
{
    if (ChkLen(len, 3))
    {
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
    }
    return 0;
}

int TsiSp003App::FA0F_ResetLogs(uint8_t *data, int len)
{
    if (ChkLen(len, 3))
    {
        auto r = ctrller.CmdResetLog(data);
        (r == APP::ERROR::AppNoError) ? Ack() : Reject(r);
    }
    return 0;
}

int TsiSp003App::FA10_SendFileInfo(uint8_t *data, int len)
{
    if (ChkLen(len, 9))
    {
        if (shake_hands_status == 2)
        {
            if (ucihw.IsUpgradeAllowed())
            {
                if (upgrade.FileInfo(data))
                {
                    db.GetUciAlarm().Push(0, "Upgrading failed: Invalid file info");
                    Reject(APP::ERROR::UserDefinedFE);
                }
                else
                {
                    Ack();
                }
            }
            else
            {
                Reject(APP::ERROR::MiNotSupported); // upgrade not allowed
            }
        }
        else
        {
            Reject(APP::ERROR::IncorrectPassword);
        }
    }
    return 0;
}

int TsiSp003App::FA11_SendFilePacket(uint8_t *data, int len)
{
    if (shake_hands_status == 2)
    {
        if (ucihw.IsUpgradeAllowed())
        {
            if (upgrade.FilePacket(data, len))
            {
                db.GetUciAlarm().Push(0, "Upgrading failed: Invalid file packet");
                Reject(APP::ERROR::UserDefinedFE);
            }
            else
            {
                Ack();
            }
        }
        else
        {
            Reject(APP::ERROR::MiNotSupported);
        }
    }
    else
    {
        Reject(APP::ERROR::IncorrectPassword);
        return 0;
    }
    return 0;
}

int TsiSp003App::FA12_StartUpgrading(uint8_t *data, int len)
{
    if (shake_hands_status == 2)
    {
        if (ucihw.IsUpgradeAllowed())
        {
            int r = upgrade.Start();
            if (r != 0)
            {
                db.GetUciAlarm().Push(0, "Upgrading failed: packet/md5 error = %d", r);
                Reject(APP::ERROR::DataChksumError);
            }
            else
            {
                db.GetUciEvent().Push(0, "Upgrading success: MD5=%s", upgrade.MD5());
                Ack();
            }
        }
        else
        {
            Reject(APP::ERROR::MiNotSupported);
        }
    }
    else
    {
        Reject(APP::ERROR::IncorrectPassword);
        return 0;
    }
    return 0;
}

APP::ERROR TsiSp003App::CheckFA20(uint8_t *pd, char *shakehands_passwd)
{
    if (shake_hands_status != 2)
    {
        return APP::ERROR::IncorrectPassword;
    }
    // check valid data
    uint32_t v;
    if (*(pd + 6) == *(pd + 5))
        return APP::ERROR::SyntaxError; // Deviceid =/= Broadcastaddr
    // v=*(pd+7);		// TmcPath
    // if(v!=1 && v!=2)	return APP::ERROR::SyntaxError;
    for (int i = 18; i <= 20; i++) // overtemp, fanOnTemp, humidity
    {
        if (*(pd + i) > 100)
            return APP::ERROR::SyntaxError;
    }
    if (*(pd + 21) >= NUMBER_OF_TZ)
        return APP::ERROR::SyntaxError;
    /* obsolete feature
    v = *(pd + 22); // default font
    if (v == 0 || v > 5)
        return APP::ERROR::SyntaxError;
    v = *(pd + 23); // default colour
    if (v == 0 || v > 9)
        return APP::ERROR::SyntaxError;
    */
    memset(shakehands_passwd, 0, 11);
    for (int i = 0; i < 10; i++)
    {
        char pc = *(pd + 26 + i);
        if (pc == 0)
        {
            break;
        }
        else if (pc < 0x20 || pc >= 0x7F)
        {
            return APP::ERROR::SyntaxError;
        }
        else
        {
            shakehands_passwd[i] = pc;
        }
    }
    uint8_t tmp0[4] = {0, 0, 0, 0};
    uint8_t tmp255[4] = {255, 255, 255, 255};
    uint8_t *pip = pd + 36;
    if (memcmp(pip, tmp0, 4) == 0 || memcmp(pip, tmp255, 4) == 0)
        return APP::ERROR::SyntaxError;

    unsigned char *pnetmask = pd + 43;
    if (memcmp(pnetmask, tmp0, 4) == 0 || memcmp(pnetmask, tmp255, 4) == 0)
        return APP::ERROR::SyntaxError;

    unsigned char *pgateway = pd + 47;
    if (memcmp(pgateway, tmp0, 4) == 0 || memcmp(pgateway, tmp255, 4) == 0)
        return APP::ERROR::SyntaxError;

    if (memcmp(pgateway, pip, 4) == 0)
        return APP::ERROR::SyntaxError;

    return APP::ERROR::AppNoError;
}

int TsiSp003App::FA20_SetUserCfg(uint8_t *data, int len)
{
    enum class FA20CODE : uint8_t
    {
        ALL_GOOD,
        RESTART,
        REBOOT,
        END_SESSION,
        NEW_PARAMETERS,
        NETWORK
    };
    if (ChkLen(len, 51))
    {
        char shakehands_passwd[11];
        APP::ERROR a = CheckFA20(data, shakehands_passwd);
        if (a != APP::ERROR::AppNoError)
        {
            Reject(a);
            return 0;
        }
        else
        {
            uint8_t *pd = data;
            auto &evt = db.GetUciEvent();
            // restart/reboot flag
            unsigned char rr_flag = 0;
            // save config
            uint32_t v;
            v = *(pd + 2);
            if (v != usercfg.SeedOffset())
            {
                evt.Push(0, "UserCfg.SeedOffset changed: 0x%02X->0x%02X", usercfg.SeedOffset(), v);
                usercfg.SeedOffset(v);
                rr_flag |= RQST_NEW_PARA;
            }
            v = Cnvt::GetU16(pd + 3);
            if (v != usercfg.PasswordOffset())
            {
                evt.Push(0, "UserCfg.PasswordOffset changed: 0x%04X->0x%04X", usercfg.PasswordOffset(), v);
                usercfg.PasswordOffset(v);
                rr_flag |= RQST_NEW_PARA;
            }
            v = *(pd + 5);
            if (v != usercfg.DeviceId())
            {
                evt.Push(0, "UserCfg.DeviceID changed: %u->%u", usercfg.DeviceId(), v);
                usercfg.DeviceId(v);
                rr_flag |= RQST_NEW_PARA;
            }
            v = *(pd + 6);
            if (v != usercfg.BroadcastId())
            {
                evt.Push(0, "UserCfg.BroadcastID changed: %u->%u", usercfg.BroadcastId(), v);
                usercfg.BroadcastId(v);
                rr_flag |= RQST_NEW_PARA;
            }

            /*
            v = *(pd + 7);
            if (v != usercfg.TmcPath())
            {
                evt.Push(0, "TMC Path changed: %u->%u . Restart to load new setting",
                            usercfg.TmcPath(), v);
                usercfg.TmcPath(v);
                rr_flag = RQST_RESTART;
            }*/

            v = Cnvt::GetU32(pd + 8);
            if (v != usercfg.TmcBaudrate())
            {
                evt.Push(0, "UserCfg.TmcBaudrate changed: %u->%u . Restart to load new setting",
                         usercfg.TmcBaudrate(), v);
                usercfg.TmcBaudrate(v);
                rr_flag |= RQST_RESTART;
            }

            v = Cnvt::GetU16(pd + 12);
            if (v != usercfg.TmcTcpPort())
            {
                evt.Push(0, "UserCfg.TmcTcpPort changed: %u->%u. Restart to load new setting",
                         usercfg.TmcTcpPort(), v);
                usercfg.TmcTcpPort(v);
                rr_flag |= RQST_RESTART;
            }

            v = Cnvt::GetU16(pd + 14);
            if (v != usercfg.SessionTimeoutSec())
            {
                evt.Push(0, "UserCfg.SessionTimeout changed: %u->%u",
                         usercfg.SessionTimeoutSec(), v);
                usercfg.SessionTimeoutSec(v);
            }

            v = Cnvt::GetU16(pd + 16);
            if (v != usercfg.DisplayTimeoutMin())
            {
                evt.Push(0, "UserCfg.DisplayTimeout changed: %u->%u",
                         usercfg.DisplayTimeoutMin(), v);
                usercfg.DisplayTimeoutMin(v);
            }

            v = *(pd + 18);
            if (v != usercfg.OverTemp())
            {
                evt.Push(0, "UserCfg.OverTemp changed: %u->%u",
                         usercfg.OverTemp(), v);
                usercfg.OverTemp(v);
            }

            v = *(pd + 19);
            if (v != usercfg.Fan1OnTemp())
            {
                evt.Push(0, "UserCfg.Fan1OnTemp changed: %u->%u",
                         usercfg.Fan1OnTemp(), v);
                usercfg.Fan1OnTemp(v);
            }
            if (v != usercfg.Fan2OnTemp())
            {
                evt.Push(0, "UserCfg.Fan2OnTemp changed: %u->%u",
                         usercfg.Fan2OnTemp(), v);
                usercfg.Fan2OnTemp(v);
            }

            v = *(pd + 20);
            if (v != usercfg.Humidity())
            {
                evt.Push(0, "UserCfg.Humidity changed: %u->%u",
                         usercfg.Humidity(), v);
                usercfg.Humidity(v);
            }

            v = *(pd + 21);
            if (v != usercfg.CityId())
            {
                evt.Push(0, "UserCfg.City changed: %u->%u",
                         usercfg.CityId(), v);
                usercfg.CityId(v);
                rr_flag |= RQST_RESTART;
            }

            /* discard DefaultFont & DefaultColour
            v = *(pd + 22);
            if (v != usercfg.DefaultFont())
            {
                evt.Push(0, "UserCfg.DefaultFont changed: %u->%u",
                         usercfg.DefaultFont(), v);
                usercfg.DefaultFont(v);
            }

            v = *(pd + 23);
            if (v != usercfg.DefaultColour())
            {
                evt.Push(0, "UserCfg.DefaultColour changed: %u->%u",
                         usercfg.DefaultColour(), v);
                usercfg.DefaultColour(v);
            }*/

            v = Cnvt::GetU16(pd + 24);
            if (v != usercfg.MultiLedFaultThreshold())
            {
                evt.Push(0, "UserCfg.MultiLedFaultThreshold changed: %u->%u",
                         usercfg.MultiLedFaultThreshold(), v);
                usercfg.MultiLedFaultThreshold(v);
            }

            if (shakehands_passwd[0] != 0)
            {
                evt.Push(0, "UserCfg.ShakehandsPassword changed");
                usercfg.ShakehandsPassword(shakehands_passwd);
            }
            v = *(pd + 40);
            if (v != usercfg.LockedMsg())
            {
                evt.Push(0, "UserCfg.LockedMsg changed: %u->%u",
                         usercfg.LockedMsg(), v);
                usercfg.LockedMsg(v);
            }

            v = *(pd + 41);
            if (v != usercfg.LockedFrm())
            {
                evt.Push(0, "UserCfg.LockedFrm changed: %u->%u",
                         usercfg.LockedFrm(), v);
                usercfg.LockedFrm(v);
            }

            v = *(pd + 42);
            if (v != usercfg.LastFrmTime())
            {
                evt.Push(0, "UserCfg.LastFrmTime changed: %u->%u",
                         usercfg.LastFrmTime(), v);
                usercfg.LastFrmTime(v);
            }

            // network settings
            auto &uciNet = db.GetUciNetwork();
            std::string eth("ETH1");
            auto net = uciNet.GetETH(eth);
            if (net != nullptr)
            {
                NetInterface nif;
                nif.proto = std::string("static");
                nif.ipaddr.Set(pd + 36);
                nif.netmask.Set(pd + 43);
                nif.gateway.Set(pd + 47);
                nif.dns = net->dns;
                uciNet.evts.clear();
                if (uciNet.SaveETH(eth, nif) == 0)
                {
                    if (!uciNet.evts.empty())
                    {
                        if (uciNet.UciCommit() == 0)
                        {
                            rr_flag |= RQST_REBOOT;
                        }
                    }
                }
            }
            txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
            txbuf[1] = FACMD_RPL_SET_USER_CFG;
            txbuf[2] = 0;
            if (rr_flag & RQST_REBOOT)
            {
                Controller::Instance().RR_flag(RQST_REBOOT);
                txbuf[2] = static_cast<uint8_t>(FA20CODE::REBOOT);
            }
            else if (rr_flag & RQST_RESTART)
            {
                Controller::Instance().RR_flag(RQST_RESTART);
                txbuf[2] = static_cast<uint8_t>(FA20CODE::RESTART);
            }
            else if (rr_flag & RQST_NEW_PARA)
            {
                txbuf[2] = static_cast<uint8_t>(FA20CODE::NEW_PARAMETERS);
            }
            Tx(txbuf, 3);
        }
    }
    return 0;
}

int TsiSp003App::FA21_RqstUserCfg(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
        txbuf[1] = FACMD_SET_USER_CFG;
        auto pt = txbuf + 2;
        *pt++ = usercfg.SeedOffset();
        pt = Cnvt::PutU16(usercfg.PasswordOffset(), pt);
        *pt++ = usercfg.DeviceId();
        *pt++ = usercfg.BroadcastId();
        *pt++ = 1; // TMC Path
        pt = Cnvt::PutU32(usercfg.TmcBaudrate(), pt);
        pt = Cnvt::PutU16(usercfg.TmcTcpPort(), pt);
        pt = Cnvt::PutU16(usercfg.SessionTimeoutSec(), pt);
        pt = Cnvt::PutU16(usercfg.DisplayTimeoutMin(), pt);
        *pt++ = usercfg.OverTemp();
        *pt++ = usercfg.Fan1OnTemp();
        *pt++ = usercfg.Humidity();
        *pt++ = usercfg.CityId();
        *pt++ = 0;                        // usercfg.DefaultFont();
        *pt++ = ucihw.GetMappedColour(0); // DefaultColour();
        pt = Cnvt::PutU16(usercfg.MultiLedFaultThreshold(), pt);
        memset(pt, 0, 10);
        pt += 10;
        auto net = db.GetUciNetwork().GetETH(std::string("ETH1"));
        memcpy(pt, net->ipaddr.ip.ipa, 4);
        pt += 4;
        *pt++ = usercfg.LockedMsg();
        *pt++ = usercfg.LockedFrm();
        *pt++ = usercfg.LastFrmTime();
        memcpy(pt, net->netmask.ip.ipa, 4);
        pt += 4;
        memcpy(pt, net->gateway.ip.ipa, 4);
        pt += 4;
        Tx(txbuf, pt - txbuf);
    }
    return 0;
}

int TsiSp003App::FA22_RqstUserExt(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
        txbuf[1] = FACMD_RPL_USER_EXT;
        auto pt = txbuf + 2;
        auto &groups = ctrller.GetGroups();
        int voltage = 0;
        int voltagemin = 0xFFFF;
        int voltagemax = 0;
        int maxTemp = 0;
        int curTemp = 0;
        int lux = 0;
        int cnt = 0;
        for (auto &g : groups)
        {
            auto &signs = g->GetSigns();
            for (auto &s : signs)
            {
                if (!s->SignErr().IsSet(DEV::ERROR::InternalCommunicationsFailure))
                {
                    auto v = s->Voltage();
                    voltage += v;
                    if (v < voltagemin)
                    {
                        voltagemin = v;
                    }
                    if (v > voltagemax)
                    {
                        voltagemax = v;
                    }
                    if (s->MaxTemp() > maxTemp)
                    {
                        maxTemp = s->MaxTemp();
                    }
                    curTemp += s->CurTemp();
                    lux += s->Lux();
                    cnt++;
                }
            }
        }
        *pt++ = maxTemp;
        *pt++ = curTemp / cnt; // avg of all current temperatures
        *pt++ = ctrller.MaxTemp();
        *pt++ = ctrller.CurTemp();
        voltage = (voltagemin < ucihw.SlaveVoltageLow()) ? voltagemin : ((voltagemax > ucihw.SlaveVoltageHigh()) ? voltagemax : (voltage / cnt));
        pt = Cnvt::PutU16(voltage, pt);
        pt = Cnvt::PutU16(lux / cnt, pt);
        char *mfcCode = ucihw.MfcCode();
        *pt++ = mfcCode[4]; // Get PCB revision from MANUFACTURER_CODE
        *pt++ = mfcCode[5]; // Get Sign type from MANUFACTURER_CODE
        memcpy(pt, FirmwareVer, 4);
        pt += 4;
        Tx(txbuf, pt - txbuf);
    }
    return 0;
}

int TsiSp003App::FA30_SignTest(uint8_t *data, int len)
{
    if (ChkLen(len, 4))
    {
        auto r = Controller::Instance().CmdSignTest(data);
        (r != APP::ERROR::AppNoError) ? Reject(r) : Ack();
    }
    return 0;
}

int TsiSp003App::FA01_SetLuminance(uint8_t *data, int len)
{
    if (ChkLen(len, 50))
    {
        uint8_t *pd = data + 2;
        for (int i = 0; i < 8; i++)
        {
            if (*pd > 23 || *(pd + 1) > 59)
            {
                Reject(APP::ERROR::SyntaxError);
                return 0;
            }
            pd += 2;
        }
        pd = data + 2;
        for (int i = 0; i < 4; i++)
        {
            int start = Cnvt::GetU16(pd);
            int stop = Cnvt::GetU16(pd + 2);
            if (start >= stop)
            {
                Reject(APP::ERROR::SyntaxError);
                return 0;
            }
            pd += 4;
        }
        uint16_t buf2[16];
        pd = data + 2 + 16;
        for (int i = 0; i < 16; i++)
        {
            buf2[i] = Cnvt::GetU16(pd);
            if (buf2[i] == 0 || (i > 0 && buf2[i - 1] >= buf2[i]))
            {
                Reject(APP::ERROR::SyntaxError);
                return 0;
            }
            pd += 2;
        }
        usercfg.DawnDusk(data + 2);
        usercfg.Luminance(buf2);
        Ack();
    }
    return 0;
}

int TsiSp003App::FA04_RqstLuminance(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
        txbuf[1] = FACMD_SET_LUMINANCE;
        auto pt = txbuf + 2;
        memcpy(pt, usercfg.DawnDusk(), 16);
        pt += 16;
        uint16_t *luminance = usercfg.Luminance();
        for (int i = 0; i < 16; i++)
        {
            pt = Cnvt::PutU16(*luminance++, pt);
        }
        Tx(txbuf, pt - txbuf);
    }
    return 0;
}

int TsiSp003App::FA02_SetExtInput(uint8_t *data, int len)
{
    if (ChkLen(len, 15))
    {
        uint8_t *pt = data + 2;
        ExtInput extin[3];
        for (int i = 0; i < 3; i++)
        {
            extin[i].dispTime = Cnvt::GetU16(pt);
            pt += 2;
        }
        for (int i = 0; i < 3; i++)
        {
            auto k = *pt++;
            if (k > 1)
            {
                Reject(APP::ERROR::SyntaxError);
                return 0;
            }
            extin[i].emergency = k;
        }
        for (int i = 0; i < 3; i++)
        {
            extin[i].reserved = *pt;
        }
        pt++;
        for (int i = 0; i < 3; i++)
        {
            auto k = *pt++;
            if (k > 1)
            {
                Reject(APP::ERROR::SyntaxError);
                return 0;
            }
            extin[i].flashingOv = k;
        }
        for (int i = 0; i < 3; i++)
        {
            usercfg.ExtInputCfgX(i, extin[i]);
        }
        auto &evt = db.GetUciEvent();
        evt.Push(0, "UserCfg.ExtInput1-3 changed");
        Ack();
    }
    return 0;
}

int TsiSp003App::FA03_RqstExtInput(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
        txbuf[1] = FACMD_SET_EXT_INPUT;
        auto pt = txbuf + 2;
        for (int i = 0; i < 3; i++)
        {
            pt = Cnvt::PutU16(usercfg.ExtInputCfgX(i).dispTime, pt);
        }
        for (int i = 0; i < 3; i++)
        {
            *pt++ = usercfg.ExtInputCfgX(i).emergency;
        }
        *pt++ = usercfg.ExtInputCfgX(0).reserved; // feedback = reserved, only report [0]
        for (int i = 0; i < 3; i++)
        {
            *pt++ = usercfg.ExtInputCfgX(i).flashingOv;
        }
        Tx(txbuf, pt - txbuf);
    }
    return 0;
}

int TsiSp003App::FAF0_ShakehandsRqst(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
        txbuf[1] = FACMD_SHAKE_REPLY;
        for (int i = 0; i < 16; i++)
        {
            shake_src[i] = random();
        }
        memcpy(txbuf + 2, shake_src, 16);
        shake_hands_status = 1;
        Tx(txbuf, 18);
    }
    return 0;
}

int TsiSp003App::FAF2_ShakehandsPasswd(uint8_t *data, int len)
{
    if (ChkLen(len, 18))
    {
        if (shake_hands_status != 1)
        {
            Reject(APP::ERROR::SyntaxError);
            return 0;
        }
        unsigned char sh_pwd[16];
        Md5_of_sh(usercfg.ShakehandsPassword(), sh_pwd);
        if (memcmp(sh_pwd, data + 2, 16) == 0)
        {
            shake_hands_status = 2;
        }
        else
        {
            Md5_of_sh("Br1ghtw@y", sh_pwd); // super password: Br1ghtw@y
            if (memcmp(sh_pwd, data + 2, 16) == 0)
            {
                shake_hands_status = 2;
            }
            else
            {
                shake_hands_status = 0;
                Reject(APP::ERROR::IncorrectPassword);
            }
        }
        Ack();
    }
    return 0;
}

int TsiSp003App::FAF5_Restart(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        if (shake_hands_status != 2)
        {
            Reject(APP::ERROR::IncorrectPassword);
        }
        else
        {
            Controller::Instance().RR_flag(RQST_RESTART);
            Ack();
        }
    }
    return 0;
}

int TsiSp003App::FAFA_Reboot(uint8_t *data, int len)
{
    if (ChkLen(len, 2))
    {
        if (shake_hands_status != 2)
        {
            Reject(APP::ERROR::IncorrectPassword);
        }
        else
        {
            Controller::Instance().RR_flag(RQST_REBOOT);
            Ack();
        }
    }
    return 0;
}

void TsiSp003App::Md5_of_sh(const char *str, unsigned char *md5)
{
    int len = 16;
    for (int i = 0; i < 10; i++)
    {
        if (*str == 0)
        {
            break;
        }
        shake_src[len++] = *str++;
    }
    MD5(shake_src, len, md5);
}

int TsiSp003App::FE_SetGuiConfig(uint8_t *data, int len)
{
    if (ChkLen(len, 23))
    {
        auto sessiont = Cnvt::GetU16(data + 14);
        auto displayt = Cnvt::GetU16(data + 17);
        auto devId = data[4];
        auto overtemp = data[9];
        auto fan1temp = data[10];
        auto fan2temp = data[11];
        auto humid = data[12];
        auto broadcastId = data[13];
        auto defaultFont = data[16];
        if (sessiont < 60 ||
            (displayt != 0 && sessiont > displayt * 60) ||
            devId == broadcastId ||
            overtemp > 99 || fan1temp > 99 || fan2temp > 99 || humid > 99 ||
            ucihw.IsFont(defaultFont) == false)
        {
            Reject(APP::ERROR::SyntaxError);
        }
        else
        {
            auto &evt = db.GetUciEvent();
            auto passwordOffset = Cnvt::GetU16(data + 1);
            if (passwordOffset != usercfg.PasswordOffset())
            {
                evt.Push(0, "UserCfg.PasswordOffset changed: %u->%u",
                         usercfg.PasswordOffset(), passwordOffset);
                usercfg.PasswordOffset(passwordOffset);
            }
            if (data[3] != usercfg.SeedOffset())
            {
                evt.Push(0, "UserCfg.SeedOffset changed: %u->%u",
                         usercfg.SeedOffset(), data[3]);
                usercfg.SeedOffset(data[3]);
            }
            if (devId != usercfg.DeviceId())
            {
                evt.Push(0, "UserCfg.DeviceId changed: %u->%u",
                         usercfg.DeviceId(), devId);
                usercfg.DeviceId(devId);
            }
            if (overtemp != usercfg.OverTemp())
            {
                evt.Push(0, "UserCfg.OverTemp changed: %u->%u",
                         usercfg.OverTemp(), overtemp);
                usercfg.OverTemp(overtemp);
            }
            if (fan1temp != usercfg.Fan1OnTemp())
            {
                evt.Push(0, "UserCfg.Fan1OnTemp changed: %u->%u",
                         usercfg.Fan1OnTemp(), fan1temp);
                usercfg.Fan1OnTemp(fan1temp);
            }
            if (fan2temp != usercfg.Fan2OnTemp())
            {
                evt.Push(0, "UserCfg.Fan2OnTemp changed: %u->%u",
                         usercfg.Fan2OnTemp(), fan2temp);
                usercfg.Fan2OnTemp(fan2temp);
            }
            if (humid != usercfg.Humidity())
            {
                evt.Push(0, "UserCfg.Humidity changed: %u->%u",
                         usercfg.Humidity(), humid);
                usercfg.Humidity(humid);
            }
            if (broadcastId != usercfg.BroadcastId())
            {
                evt.Push(0, "UserCfg.BroadcastId changed: %u->%u",
                         usercfg.BroadcastId(), broadcastId);
                usercfg.BroadcastId(broadcastId);
            }
            /*
            if (defaultFont != usercfg.DefaultFont())
            {
                evt.Push(0, "UserCfg.DefaultFont changed: %u->%u",
                         usercfg.DefaultFont(), defaultFont);
                usercfg.DefaultFont(defaultFont);
            }*/
            if (sessiont != usercfg.SessionTimeoutSec())
            {
                evt.Push(0, "UserCfg.SessionTimeout changed: %u->%u",
                         usercfg.SessionTimeoutSec(), sessiont);
                usercfg.SessionTimeoutSec(sessiont);
            }
            if (displayt != usercfg.DisplayTimeoutMin())
            {
                evt.Push(0, "UserCfg.DisplayTimeout changed: %u->%u",
                         usercfg.DisplayTimeoutMin(), displayt);
                usercfg.DisplayTimeoutMin(displayt);
            }
            // after DisplayTimeout, there are 4 bytes, don't know the meaning
            Ack();
        }
    }
    return 0;
}

int TsiSp003App::FF_RqstGuiConfig(uint8_t *data, int len)
{
    if (ChkLen(len, 1))
    {
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFF);
        uint8_t *p = txbuf + 1;
        p = Cnvt::PutU16(usercfg.PasswordOffset(), p);    // password offset
        *p++ = usercfg.SeedOffset();                      // seed offset
        *p++ = usercfg.DeviceId();                        // device id
        p = Cnvt::PutU16(5, p);                           // conspicuity( flasher ) ON time : 5/10 seconds
        p = Cnvt::PutU16(5, p);                           // conspicuity( flasher ) OFF time : 5/10 seconds
        *p++ = usercfg.OverTemp();                        // over temperature
        *p++ = usercfg.Fan1OnTemp();                      // fan 1 on temperature
        *p++ = usercfg.Fan2OnTemp();                      // fan 2 on temperature
        *p++ = usercfg.Humidity();                        // Humidity
        *p++ = usercfg.BroadcastId();                     // broadcast address
        p = Cnvt::PutU16(usercfg.SessionTimeoutSec(), p); // session time out
        auto &groups = Controller::Instance().GetGroups();
        int faultleds = 0;
        int maxTemp = 0;
        int curTemp = 0;
        int lux = 0;
        int cnt = ucihw.NumberOfSigns();
        for (auto &g : groups)
        {
            auto &signs = g->GetSigns();
            for (auto &s : signs)
            {
                faultleds += s->FaultLedCnt();
                if (s->MaxTemp() > maxTemp)
                {
                    maxTemp = s->MaxTemp();
                }
                curTemp += s->CurTemp();
                lux += s->Lux();
            }
        }
        *p++ = curTemp / cnt;                             // avg of all current temperatures
        p = Cnvt::PutU16(lux / cnt, p);                   // avg of all
        *p++ = maxTemp;                                   // max of all max temperatures
        *p++ = (faultleds > 255) ? 255 : faultleds;       // pixel on fault
        *p++ = 0;                                         // DefaultFont                  //
        p = Cnvt::PutU16(usercfg.DisplayTimeoutMin(), p); // display time out
        *p++ = 0;                                         //	    GUIconfigure.PARA.BYTE.define_modem=0;		//
        p = Cnvt::PutU16(0, p);                           // light sensor 2
        *p++ = 'V';                                       // GUIconfigure.PARA.BYTE.device_type='V';		// "V"
        *p++ = 'B';                                       // GUIconfigure.PARA.BYTE.device_operation='B';	// "B"
        *p++ = ucihw.MaxConspicuity();                    // conspicuity
        *p++ = ucihw.MaxFont();                           // max. number of fonts
        *p++ = ucihw.GetMappedColour(0);                  // user.DefaultColour();                // 09
        *p++ = 0;                                         // GUIconfigure.PARA.BYTE.max_template=0;		// 00
        *p++ = 1;                                         // GUIconfigure.PARA.BYTE.wk1=1;                // 01
        *p++ = 0;                                         // GUIconfigure.PARA.BYTE.group_offset=0;		// 00
        *p++ = 'D';                                       // GUIconfigure.PARA.BYTE.wk2='D';                // 0x44 'D'
        *p++ = 0;                                         // GUIconfigure.PARA.BYTE.group_length=0;		// 00
        *p++ = 1;                                         // GUIconfigure.PARA.BYTE.wk3=1;                // 01
        *p++ = 1;                                         // GUIconfigure.PARA.BYTE.group_data=1;			// 01
        Tx(txbuf, 39);
    }
    return 0;
}
