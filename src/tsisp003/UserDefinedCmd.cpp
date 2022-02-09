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
    default:
        Reject(APP::ERROR::UnknownMi);
    }
    return 0;
}

int TsiSp003App::FA0A_RetrieveLogs(uint8_t *data, int len)
{
    if (ChkLen(len, 3))
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
    }
    return 0;
}

int TsiSp003App::FA0F_ResetLogs(uint8_t *data, int len)
{
    if (ChkLen(len, 3))
    {
        auto &db = DbHelper::Instance();
        if (!db.GetUciProd().IsResetLogAllowed())
        {
            Reject(APP::ERROR::MiNotSupported);
            return 0;
        }
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
    }
    return 0;
}

int TsiSp003App::FA10_SendFileInfo(uint8_t *data, int len)
{
    if (ChkLen(len, 9))
    {
        if (shake_hands_status == 2)
        {
            if (db.GetUciProd().IsUpgradeAllowed())
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
        if (db.GetUciProd().IsUpgradeAllowed())
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
        if (db.GetUciProd().IsUpgradeAllowed())
        {
            if (upgrade.Start())
            {
                db.GetUciAlarm().Push(0, "Upgrading failed: zip/md5 error");
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
    //v=*(pd+7);		// TmcPath
    //if(v!=1 && v!=2)	return APP::ERROR::SyntaxError;
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
            auto &user = DbHelper::Instance().GetUciUser();
            auto &evt = DbHelper::Instance().GetUciEvent();
            // restart/reboot flag
            unsigned char rr_flag = 0;
            // save config
            uint32_t v;
            v = *(pd + 2);
            if (v != user.SeedOffset())
            {
                evt.Push(0, "User.SeedOffset changed: 0x%02X->0x%02X", user.SeedOffset(), v);
                user.SeedOffset(v);
            }
            v = Cnvt::GetU16(pd + 3);
            if (v != user.PasswordOffset())
            {
                evt.Push(0, "User.PasswordOffset changed: 0x%04X->0x%04X", user.PasswordOffset(), v);
                user.PasswordOffset(v);
            }
            v = *(pd + 5);
            if (v != user.DeviceId())
            {
                evt.Push(0, "User.DeviceID changed: %u->%u", user.DeviceId(), v);
                user.DeviceId(v);
            }
            v = *(pd + 6);
            if (v != user.BroadcastId())
            {
                evt.Push(0, "User.BroadcastID changed: %u->%u", user.BroadcastId(), v);
                user.BroadcastId(v);
            }

            /*
            v = *(pd + 7);
            if (v != user.TmcPath())
            {
                evt.Push(0, "TMC Path changed: %u->%u . Restart to load new setting",
                            user.TmcPath(), v);
                user.TmcPath(v);
                rr_flag = RQST_RESTART;
            }*/

            v = Cnvt::GetU32(pd + 8);
            if (v != user.Baudrate())
            {
                evt.Push(0, "User.Baudrate changed: %u->%u . Restart to load new setting",
                         user.Baudrate(), v);
                user.Baudrate(v);
                rr_flag |= RQST_RESTART;
            }

            v = Cnvt::GetU16(pd + 12);
            if (v != user.SvcPort())
            {
                evt.Push(0, "User.SvcPort changed: %u->%u. Restart to load new setting",
                         user.SvcPort(), v);
                user.SvcPort(v);
                rr_flag |= RQST_RESTART;
            }

            v = Cnvt::GetU16(pd + 14);
            if (v != user.SessionTimeout())
            {
                evt.Push(0, "User.SessionTimeout changed: %u->%u",
                         user.SessionTimeout(), v);
                user.SessionTimeout(v);
            }

            v = Cnvt::GetU16(pd + 16);
            if (v != user.DisplayTimeout())
            {
                evt.Push(0, "User.DisplayTimeout changed: %u->%u",
                         user.DisplayTimeout(), v);
                user.DisplayTimeout(v);
            }

            v = *(pd + 18);
            if (v != user.OverTemp())
            {
                evt.Push(0, "User.OverTemp changed: %u->%u",
                         user.OverTemp(), v);
                user.OverTemp(v);
            }

            v = *(pd + 19);
            if (v != user.Fan1OnTemp())
            {
                evt.Push(0, "User.Fan1OnTemp changed: %u->%u",
                         user.Fan1OnTemp(), v);
                user.Fan1OnTemp(v);
            }
            if (v != user.Fan2OnTemp())
            {
                evt.Push(0, "User.Fan2OnTemp changed: %u->%u",
                         user.Fan2OnTemp(), v);
                user.Fan2OnTemp(v);
            }

            v = *(pd + 20);
            if (v != user.Humidity())
            {
                evt.Push(0, "User.Humidity changed: %u->%u",
                         user.Humidity(), v);
                user.Humidity(v);
            }

            v = *(pd + 21);
            if (v != user.Tz())
            {
                evt.Push(0, "User.Timezone changed: %u->%u",
                         user.Tz(), v);
                user.Tz(v);
                rr_flag |= RQST_RESTART;
            }

            /* discard DefaultFont & DefaultColour
            v = *(pd + 22);
            if (v != user.DefaultFont())
            {
                evt.Push(0, "User.DefaultFont changed: %u->%u",
                         user.DefaultFont(), v);
                user.DefaultFont(v);
            }

            v = *(pd + 23);
            if (v != user.DefaultColour())
            {
                evt.Push(0, "User.DefaultColour changed: %u->%u",
                         user.DefaultColour(), v);
                user.DefaultColour(v);
            }*/

            v = Cnvt::GetU16(pd + 24);
            if (v != user.MultiLedFaultThreshold())
            {
                evt.Push(0, "User.MultiLed changed: %u->%u",
                         user.MultiLedFaultThreshold(), v);
                user.MultiLedFaultThreshold(v);
            }

            if (shakehands_passwd[0] != 0)
            {
                evt.Push(0, "User.ConfigPassword changed");
                user.ShakehandsPassword(shakehands_passwd);
            }
            v = *(pd + 40);
            if (v != user.LockedMsg())
            {
                evt.Push(0, "User.LockedMsg changed: %u->%u",
                         user.LockedMsg(), v);
                user.LockedMsg(v);
            }

            v = *(pd + 41);
            if (v != user.LockedFrm())
            {
                evt.Push(0, "User.LockedFrm changed: %u->%u",
                         user.LockedFrm(), v);
                user.LockedFrm(v);
            }

            v = *(pd + 42);
            if (v != user.LastFrmOn())
            {
                evt.Push(0, "User.LastFrmOn changed: %u->%u",
                         user.LastFrmOn(), v);
                user.LastFrmOn(v);
            }

            // network settings
            auto &net = DbHelper::Instance().GetUciNetwork();
            uint8_t *pip1 = pd + 36;
            uint8_t *nip1 = net.Ipaddr();
            int m1 = memcmp(pip1, nip1, 4);
            uint8_t *pip2 = pd + 43;
            uint8_t *nip2 = net.Netmask();
            int m2 = memcmp(pip2, nip2, 4);
            uint8_t *pip3 = pd + 47;
            uint8_t *nip3 = net.Gateway();
            int m3 = memcmp(pip3, nip3, 4);
            if (m1 != 0 || m2 != 0 || m3 != 0)
            {
                char ipbuf[64];
                auto printip = [&ipbuf](const char *str, uint8_t *old_n, uint8_t *new_p) -> void
                {
                    sprintf(ipbuf, "network.%s changed: %u.%u.%u.%u->%u.%u.%u.%u",
                            str, old_n[0], old_n[1], old_n[2], old_n[3], new_p[0], new_p[1], new_p[2], new_p[3]);
                };
                if (m1 != 0)
                {
                    printip("ipaddr", nip1, pip1);
                    evt.Push(0, ipbuf);
                    net.Ipaddr(pip1);
                }
                if (m2 != 0)
                {
                    printip("netmask", nip2, pip2);
                    evt.Push(0, ipbuf);
                    net.Netmask(pip2);
                }
                if (m3 != 0)
                {
                    printip("gateway", nip3, pip3);
                    evt.Push(0, ipbuf);
                    net.Gateway(pip3);
                }
                rr_flag |= RQST_NETWORK;
            }
            txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
            txbuf[1] = FACMD_RPL_SET_USER_CFG;
            if (rr_flag == 0)
            {
                txbuf[2] = 0;
            }
            else
            {
                if (rr_flag & RQST_NETWORK)
                {
                    Controller::Instance().RR_flag(rr_flag);
                    txbuf[2] = 5;
                }
                if (rr_flag & RQST_RESTART)
                {
                    txbuf[2] = 1;
                }
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
        auto &user = DbHelper::Instance().GetUciUser();
        *pt++ = user.SeedOffset();
        pt = Cnvt::PutU16(user.PasswordOffset(), pt);
        *pt++ = user.DeviceId();
        *pt++ = user.BroadcastId();
        *pt++ = 1; // TMC Path
        pt = Cnvt::PutU32(user.Baudrate(), pt);
        pt = Cnvt::PutU16(user.SvcPort(), pt);
        pt = Cnvt::PutU16(user.SessionTimeout(), pt);
        pt = Cnvt::PutU16(user.DisplayTimeout(), pt);
        *pt++ = user.OverTemp();
        *pt++ = user.Fan1OnTemp();
        *pt++ = user.Humidity();
        *pt++ = user.Tz();
        *pt++ = 0;                                                    //user.DefaultFont();
        *pt++ = DbHelper::Instance().GetUciProd().GetMappedColour(0); //DefaultColour();
        pt = Cnvt::PutU16(user.MultiLedFaultThreshold(), pt);
        memset(pt, 0, 10);
        pt += 10;
        auto &net = DbHelper::Instance().GetUciNetwork();
        memcpy(pt, net.Ipaddr(), 4);
        pt += 4;
        *pt++ = user.LockedMsg();
        *pt++ = user.LockedFrm();
        *pt++ = user.LastFrmOn();
        memcpy(pt, net.Netmask(), 4);
        pt += 4;
        memcpy(pt, net.Gateway(), 4);
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
        auto &prod = db.GetUciProd();
        auto &ctrl = Controller::Instance();
        auto &groups = ctrl.GetGroups();
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
        *pt++ = ctrl.MaxTemp();
        *pt++ = ctrl.CurTemp();
        voltage = (voltagemin < prod.SlaveVoltageLow()) ? voltagemin : ((voltagemax > prod.SlaveVoltageHigh()) ? voltagemax : (voltage / cnt));
        pt = Cnvt::PutU16(voltage, pt);
        pt = Cnvt::PutU16(lux / cnt, pt);
        char *mfcCode = DbHelper::Instance().GetUciProd().MfcCode();
        *pt++ = mfcCode[4]; // Get PCB revision from MANUFACTURER_CODE
        *pt++ = mfcCode[5]; // Get Sign type from MANUFACTURER_CODE
        memcpy(pt, FirmwareVer, 4);
        pt += 4;
        Tx(txbuf, pt - txbuf);
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
        auto &user = DbHelper::Instance().GetUciUser();
        user.DawnDusk(data + 2);
        user.Luminance(buf2);
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
        auto &user = DbHelper::Instance().GetUciUser();
        memcpy(pt, user.DawnDusk(), 16);
        pt += 16;
        uint16_t *luminance = user.Luminance();
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
        ExtSw extsw[3];
        for (int i = 0; i < 3; i++)
        {
            extsw[i].dispTime = Cnvt::GetU16(pt);
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
            extsw[i].emergency = k;
        }
        for (int i = 0; i < 3; i++)
        {
            extsw[i].reserved = *pt;
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
            extsw[i].flashingOv = k;
        }
        auto &user = DbHelper::Instance().GetUciUser();
        for (int i = 0; i < 3; i++)
        {
            user.ExtSwCfgX(i, extsw[i]);
        }
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
        auto &user = DbHelper::Instance().GetUciUser();
        for (int i = 0; i < 3; i++)
        {
            pt = Cnvt::PutU16(user.ExtSwCfgX(i).dispTime, pt);
        }
        for (int i = 0; i < 3; i++)
        {
            *pt++ = user.ExtSwCfgX(i).emergency;
        }
        *pt++ = user.ExtSwCfgX(0).reserved; // feedback = reserved, only report [0]
        for (int i = 0; i < 3; i++)
        {
            *pt++ = user.ExtSwCfgX(i).flashingOv;
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
        auto &user = DbHelper::Instance().GetUciUser();
        Md5_of_sh(user.ShakehandsPassword(), sh_pwd);
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
        auto &prod = DbHelper::Instance().GetUciProd();
        auto &user = DbHelper::Instance().GetUciUser();
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
            prod.IsFont(defaultFont) == false)
        {
            Reject(APP::ERROR::SyntaxError);
        }
        else
        {
            auto &evt = DbHelper::Instance().GetUciEvent();
            auto passwordOffset = Cnvt::GetU16(data + 1);
            if (passwordOffset != user.PasswordOffset())
            {
                evt.Push(0, "User.PasswordOffset changed: %u->%u",
                         user.PasswordOffset(), passwordOffset);
                user.PasswordOffset(passwordOffset);
            }
            if (data[3] != user.SeedOffset())
            {
                evt.Push(0, "User.SeedOffset changed: %u->%u",
                         user.SeedOffset(), data[3]);
                user.SeedOffset(data[3]);
            }
            if (devId != user.DeviceId())
            {
                evt.Push(0, "User.DeviceId changed: %u->%u",
                         user.DeviceId(), devId);
                user.DeviceId(devId);
            }
            if (overtemp != user.OverTemp())
            {
                evt.Push(0, "User.OverTemp changed: %u->%u",
                         user.OverTemp(), overtemp);
                user.OverTemp(overtemp);
            }
            if (fan1temp != user.Fan1OnTemp())
            {
                evt.Push(0, "User.Fan1OnTemp changed: %u->%u",
                         user.Fan1OnTemp(), fan1temp);
                user.Fan1OnTemp(fan1temp);
            }
            if (fan2temp != user.Fan2OnTemp())
            {
                evt.Push(0, "User.Fan2OnTemp changed: %u->%u",
                         user.Fan2OnTemp(), fan2temp);
                user.Fan2OnTemp(fan2temp);
            }
            if (humid != user.Humidity())
            {
                evt.Push(0, "User.Humidity changed: %u->%u",
                         user.Humidity(), humid);
                user.Humidity(humid);
            }
            if (broadcastId != user.BroadcastId())
            {
                evt.Push(0, "User.BroadcastId changed: %u->%u",
                         user.BroadcastId(), broadcastId);
                user.BroadcastId(broadcastId);
            }
            /*
            if (defaultFont != user.DefaultFont())
            {
                evt.Push(0, "User.DefaultFont changed: %u->%u",
                         user.DefaultFont(), defaultFont);
                user.DefaultFont(defaultFont);
            }*/
            if (sessiont != user.SessionTimeout())
            {
                evt.Push(0, "User.SessionTimeout changed: %u->%u",
                         user.SessionTimeout(), sessiont);
                user.SessionTimeout(sessiont);
            }
            if (displayt != user.DisplayTimeout())
            {
                evt.Push(0, "User.DisplayTimeout changed: %u->%u",
                         user.DisplayTimeout(), displayt);
                user.DisplayTimeout(displayt);
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
        auto &prod = DbHelper::Instance().GetUciProd();
        auto &user = DbHelper::Instance().GetUciUser();
        txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFF);
        uint8_t *p = txbuf + 1;
        p = Cnvt::PutU16(user.PasswordOffset(), p); // password offset
        *p++ = user.SeedOffset();                   // seed offset
        *p++ = user.DeviceId();                     // device id
        p = Cnvt::PutU16(5, p);                     // conspicuity( flasher ) ON time : 5/10 seconds
        p = Cnvt::PutU16(5, p);                     // conspicuity( flasher ) OFF time : 5/10 seconds
        *p++ = user.OverTemp();                     // over temperature
        *p++ = user.Fan1OnTemp();                   // fan 1 on temperature
        *p++ = user.Fan2OnTemp();                   // fan 2 on temperature
        *p++ = user.Humidity();                     // Humidity
        *p++ = user.BroadcastId();                  // broadcast address
        p = Cnvt::PutU16(user.SessionTimeout(), p); // session time out
        auto &groups = Controller::Instance().GetGroups();
        int faultleds = 0;
        int maxTemp = 0;
        int curTemp = 0;
        int lux = 0;
        int cnt = db.GetUciProd().NumberOfSigns();
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
        *p++ = curTemp / cnt;                       // avg of all current temperatures
        p = Cnvt::PutU16(lux / cnt, p);             // avg of all
        *p++ = maxTemp;                             // max of all max temperatures
        *p++ = (faultleds > 255) ? 255 : faultleds; // pixel on fault
        *p++ = 0;                                   //DefaultFont                  //
        p = Cnvt::PutU16(user.DisplayTimeout(), p); // display time out
        *p++ = 0;                                   //	    GUIconfigure.PARA.BYTE.define_modem=0;		//
        p = Cnvt::PutU16(0, p);                     // light sensor 2
        *p++ = 'V';                                 // GUIconfigure.PARA.BYTE.device_type='V';		// "V"
        *p++ = 'B';                                 //GUIconfigure.PARA.BYTE.device_operation='B';	// "B"
        *p++ = prod.MaxConspicuity();               // conspicuity
        *p++ = prod.MaxFont();                      // max. number of fonts
        *p++ = prod.GetMappedColour(0);             //user.DefaultColour();                // 09
        *p++ = 0;                                   //GUIconfigure.PARA.BYTE.max_template=0;		// 00
        *p++ = 1;                                   //GUIconfigure.PARA.BYTE.wk1=1;                // 01
        *p++ = 0;                                   //GUIconfigure.PARA.BYTE.group_offset=0;		// 00
        *p++ = 'D';                                 //GUIconfigure.PARA.BYTE.wk2='D';                // 0x44 'D'
        *p++ = 0;                                   //GUIconfigure.PARA.BYTE.group_length=0;		// 00
        *p++ = 1;                                   //GUIconfigure.PARA.BYTE.wk3=1;                // 01
        *p++ = 1;                                   //GUIconfigure.PARA.BYTE.group_data=1;			// 01
        Tx(txbuf, 39);
    }
    return 0;
}
