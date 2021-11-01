#include <cstdlib>
#include <openssl/md5.h>
#include <tsisp003/TsiSp003App.h>


extern const char *FirmwareMajorVer;
extern const char *FirmwareMinorVer;

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
    default:
        Reject(APP::ERROR::SyntaxError);
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

APP::ERROR TsiSp003App::CheckFA20(uint8_t *pd, char * shakehands_passwd)
{
    if (shake_hands_status != 2)
    {
        return (APP::ERROR::IncorrectPassword);
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
    v = *(pd + 22); // default font
    if (v == 0 || v > 5)
        return APP::ERROR::SyntaxError;
    v = *(pd + 23); // default colour
    if (v == 0 || v > 9)
        return APP::ERROR::SyntaxError;
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
    return APP::ERROR::AppNoError;

    unsigned char tmp0[4] = {0, 0, 0, 0};
    unsigned char tmp255[4] = {255, 255, 255, 255};
    unsigned char *pip = pd + 36;
    if (memcmp(pip, tmp0, 4) == 0 || memcmp(pip, tmp255, 4) == 0)
        return APP::ERROR::SyntaxError;

    unsigned char *pnetmask = pd + 43;
    if (memcmp(pnetmask, tmp0, 4) == 0 || memcmp(pnetmask, tmp255, 4) == 0)
        return APP::ERROR::SyntaxError;

    unsigned char *pgateway = pd + 47;
    if (memcmp(pgateway, tmp0, 4) == 0 || memcmp(pgateway, tmp255, 4) == 0)
        return APP::ERROR::SyntaxError;
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
        }
        else
        {
            uint8_t *pd = data;
            auto & user = DbHelper::Instance().GetUciUser();
            auto & evt =  DbHelper::Instance().GetUciEvent();
             
            // restart/reboot flag
            unsigned char rr_flag{0}, RQST_NEXT_SESSION{1}, RQST_END_SESSION{1}, RQST_RESTART{2};
            user.UserOpen();
            // save config
            uint32_t v;
            v = *(pd + 2);
            if (v != user.SeedOffset())
            {
                evt.Push(0, "User.SeedOffset changed: 0x%02X->0x%02X . End current session.",
                            user.SeedOffset(), v);
                user.SeedOffset(v);
                rr_flag = RQST_NEXT_SESSION;
            }
            v = Cnvt::GetU16(pd + 3);
            if (v != user.PasswordOffset())
            {
                evt.Push(0, "User.PasswordOffset changed: 0x%04X->0x%04X . End current session.",
                            user.PasswordOffset(), v);
                user.PasswordOffset(v);
                rr_flag = RQST_NEXT_SESSION;
            }

            v = *(pd + 5);
            if (v != user.DeviceId())
            {
                evt.Push(0, "User.DeviceID changed: %u->%u . End current session.", user.DeviceId(), v);
                user.DeviceId(v);
                rr_flag = RQST_END_SESSION;
            }

            v = *(pd + 6);
            if (v != user.BroadcastId())
            {
                evt.Push(0, "User.BroadcastID changed: %u->%u . End current session.", user.BroadcastId(), v);
                user.BroadcastId(v);
                rr_flag = RQST_END_SESSION;
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
                rr_flag = RQST_RESTART;
            }

            v = Cnvt::GetU16(pd + 12);
            if (v != user.SvcPort())
            {
                evt.Push(0, "User.SvcPort changed: %u->%u. Restart to load new setting",
                            user.SvcPort(), v);
                user.SvcPort(v);
                rr_flag = RQST_RESTART;
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
            }

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
            }

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
                evt.Push(0, "user.LockedMsg changed: %u->%u",
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
            user.UserClose();
            #if 0
            // TODO network settings
            unsigned char *network;
            network = user.NetworkIpaddr();
            if (memcmp(pip, network, 4) != 0)
            {
                evt.Push(0, "network.ipaddr changed: %u.%u.%u.%u-> %u.%u.%u.%u. Reboot to load new setting",
                            *network, *(network + 1), *(network + 2), *(network + 3), *pip, *(pip + 1), *(pip + 2), *(pip + 3));
                user.NetworkIpaddr(pip);
                rr_flag = RQST_REBOOT;
            }

            network = user.NetworkNetmask();
            if (memcmp(pnetmask, network, 4) != 0)
            {
                evt.Push(0, "network.netmask changed: %u.%u.%u.%u-> %u.%u.%u.%u. Reboot to load new setting",
                            *network, *(network + 1), *(network + 2), *(network + 3), *pnetmask, *(pnetmask + 1), *(pnetmask + 2), *(pnetmask + 3));
                user.NetworkNetmask(pnetmask);
                rr_flag = RQST_REBOOT;
            }

            network = user.NetworkGateway();
            if (memcmp(pgateway, network, 4) != 0)
            {
                evt.Push(0, "network.gateway changed: %u.%u.%u.%u-> %u.%u.%u.%u. Reboot to load new setting",
                            *network, *(network + 1), *(network + 2), *(network + 3), *pgateway, *(pgateway + 1), *(pgateway + 2), *(pgateway + 3));
                user.NetworkGateway(pgateway);
                rr_flag = RQST_REBOOT;
            }
            if (rr_flag == RQST_END_SESSION)
            {
                DisplayTimeout_StartCount(user.DisplayTimeout()); // renew display time
                processCtrlFaultErrState(DISPLAYTIMEOUT, 0);              // Error = 0x1C
                // Session_StartCount(user.SessionTimeout()); // renew session time
                processCtrlFaultErrState(SESSIONTIMEOUT, 0); // Error = 0x02
                _Session_END();                              // SessionTimeout disabled in _Session_END
            }
            #endif
            txbuf[0] = static_cast<uint8_t>(MI::CODE::UserDefinedCmdFA);
            txbuf[1] = FACMD_RPL_SET_USER_CFG;
            txbuf[2] = rr_flag;
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
        *pt++ = user.DefaultFont();
        *pt++ = user.DefaultColour();
        pt = Cnvt::PutU16(user.MultiLedFaultThreshold(), pt);
        memset(pt, 0, 10);
        pt += 10;
        // ip
        *pt++ = 192;
        *pt++ = 168;
        *pt++ = 0;
        *pt++ = 1;
        *pt++ = user.LockedMsg();
        *pt++ = user.LockedFrm();
        *pt++ = user.LastFrmOn();
        // netmask
        *pt++ = 255;
        *pt++ = 255;
        *pt++ = 255;
        *pt++ = 0;
        // gateway
        *pt++ = 192;
        *pt++ = 168;
        *pt++ = 0;
        *pt++ = 1;
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
        auto &ctrl = Controller::Instance();
        auto sign = ctrl.GetGroup(1)->GetSign(1);
        *pt++ = sign->MaxTemp();
        *pt++ = sign->CurTemp();
        *pt++ = ctrl.MaxTemp();
        *pt++ = ctrl.CurTemp();
        pt = Cnvt::PutU16(sign->Voltage(), pt);
        pt = Cnvt::PutU16(sign->Lux(), pt);
        char *v = DbHelper::Instance().GetUciProd().MfcCode() + 4;
        *pt++ = *v;       // Get PCB revision from MANUFACTURER_CODE
        *pt++ = *(v + 1); // Get Sign type from MANUFACTURER_CODE
        *pt++ = *FirmwareMajorVer;
        *pt++ = *(FirmwareMajorVer + 1);
        *pt++ = *FirmwareMinorVer;
        *pt++ = *(FirmwareMinorVer + 1);
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
        user.UserOpen();
        user.DawnDusk(data + 2);
        user.Luminance(buf2);
        user.UserClose();
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
        user.UserOpen();
        for (int i = 0; i < 3; i++)
        {
            user.ExtSwCfgX(i, &extsw[i]);
        }
        user.UserClose();
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
        ExtSw *pextsw[3];
        for (int i = 0; i < 3; i++)
        {
            pextsw[i] = user.ExtSwCfgX(i);
        }
        for (int i = 0; i < 3; i++)
        {
            pt = Cnvt::PutU16(pextsw[i]->dispTime, pt);
        }
        for (int i = 0; i < 3; i++)
        {
            *pt++ = pextsw[i]->emergency;
        }
        *pt++ = pextsw[0]->reserved; // feedback = msg345_reserved, only report MSG[0/3]
        for (int i = 0; i < 3; i++)
        {
            *pt++ = pextsw[i]->flashingOv;
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
