#include <cstdarg>
#include <cstdio>
#include <tsisp003/TsiSp003App.h>
#include <sign/Controller.h>
#include <module/DS3231.h>
#include <module/MyDbg.h>

using namespace Utils;

TsiSp003App::TsiSp003App()
    : db(DbHelper::Instance()),
      ctrller(Controller::Instance())
{
    rejectStr[0] = '\0';
}

TsiSp003App::~TsiSp003App()
{
}

int TsiSp003App::Rx(uint8_t *data, int len)
{
    micode = *data;
    auto mi = static_cast<MI::CODE>(micode);
    switch (mi)
    {
    case MI::CODE::StartSession:
        StartSession(data, len);
        break;
    case MI::CODE::Password:
        Password(data, len);
        break;
    case MI::CODE::EndSession:
        EndSession(data, len);
        break;
    case MI::CODE::UpdateTime:
        UpdateTime(data, len);
        break;
    case MI::CODE::HARStatusReply:
    case MI::CODE::HARSetVoiceDataIncomplete:
    case MI::CODE::HARSetVoiceDataComplete:
    case MI::CODE::HARSetStrategy:
    case MI::CODE::HARActivateStrategy:
    case MI::CODE::HARSetPlan:
    case MI::CODE::HARRequestStoredVSP:
    case MI::CODE::HARSetVoiceDataACK:
    case MI::CODE::HARSetVoiceDataNAK:
    case MI::CODE::EnvironmentalWeatherStatusReply:
    case MI::CODE::RequestEnvironmentalWeatherValues:
    case MI::CODE::EnvironmentalWeatherValues:
    case MI::CODE::EnvironmentalWeatherThresholdDefinition:
    case MI::CODE::RequestThresholdDefinition:
    case MI::CODE::RequestEnvironmentalWeatherEventLog:
    case MI::CODE::EnvironmentalWeatherEventLogReply:
    case MI::CODE::ResetEnvironmentalWeatherEventLog:
        Reject(APP::ERROR::MiNotSupported);
        break;
    default:
        return UserDefinedCmd(data, len);
    }
    return 0;
}

void TsiSp003App::ClrRx()
{
    appErr = APP::ERROR::AppNoError;
}

void TsiSp003App::Reject(APP::ERROR error)
{
    uint8_t buf[3];
    buf[0] = static_cast<uint8_t>(MI::CODE::Reject);
    buf[1] = micode;
    buf[2] = static_cast<uint8_t>(error);
    Tx(buf, 3);
    char str[64 + 4];
    if (rejectStr[0] != '\0')
    {
        snprintf(str, 64+4-1, " :%s\n", rejectStr);
        rejectStr[0] = '\0';
    }
    else
    {
        sprintf(str, "\n");
    }
    PrintDbg(DBG_LOG, "Reject: MI=0x%02X(%s), Error=0x%02X(%s)%s", buf[1], MI::ToStr(buf[1]), buf[2], APP::ToStr(buf[2]), str);
    /*
    ACE_DEBUG((LM_DEBUG, "%s!!! cmd_Reject - error code %d : %s", LocaltimeStr(), apperror, apperrtomsg(apperror)));
	unsigned char logapperror[]={
		SNTXERR, LNGTHERR, DTCHKSMERR, NONASCIICHAR, FRAMETOOLARGE, UNDEFINEDSIGNNO,
		FONTNOTSUPPORTED, COLOURNOTSUPPORTED, PFACILITYSWOVERRIDE, CONSPICUITYNOTSUP,
		SIZEMISMATCH, FRAMETOOSMALL, INCORRECTPASSWORD};
	for(int i=0;i<sizeof(logapperror)/sizeof(logapperror[0]);i++)
	{
		if(apperror == logapperror[i])
		{
			addLogEntry(EVENT,"cmd_Reject - error code %d : %s", apperror, apperrtomsg(apperror));
			break;
		}
	}
	protocolp->updateApperror(apperror);
    */
}

void TsiSp003App::SetRejectStr(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(rejectStr, 63, fmt, args);
    va_end(args);
}

void TsiSp003App::Ack()
{
    uint8_t buf[2];
    buf[0] = static_cast<uint8_t>(MI::CODE::Ack);
    buf[1] = micode;
    Tx(buf, 2);
}

void TsiSp003App::StartSession(uint8_t *data, int len)
{
    if (ChkLen(len, 1) == false || session == nullptr)
        return;
    uint8_t seed = rand();
    session->Session(ISession::SESSION::START);
    session->Seed(seed);
    uint8_t buf[2];
    buf[0] = static_cast<uint8_t>(MI::CODE::PasswordSeed);
    buf[1] = seed;
    Tx(buf, 2);
}

void TsiSp003App::Password(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false || session == nullptr)
        return;
    if (session->Session() != ISession::SESSION::START)
    {
        SetRejectStr("Expect StartSession before Password");
        Reject(APP::ERROR::SyntaxError);
        return;
    }
    uint16_t pass = data[1] * 0x100 + data[2];
    uint16_t expp = MakePassword();
    if (pass == expp)
    {
        //SetStatusLed(1);
        Ack();
        // !!! first Ack, then set online
        // 'cause nr&ns related to SESSION::ON_LINE
        session->Session(ISession::SESSION::ON_LINE);
    }
    else
    {
        session->Session(ISession::SESSION::OFF_LINE);
        SetRejectStr("Got[%04X]Expect[%04X]", pass, expp);
        Reject(APP::ERROR::SyntaxError);
        //_Session_END();
        /*	Session_StartCount(INT_MAX-1);
	SetStatusLed(0);
	protocolp->ClearOnline();
	protocolp->ClearNRnNS();  // clear NR
	shake_hands_status = 0;*/
        //cmd_Reject(p, SNTXERR);	  // 0x02:Syntax error in command
        //addLogEntry(EVENT, "CMD: Password : Wrong password, start session failed");
    }
}

void TsiSp003App::EndSession(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 1) || session == nullptr)
        return;
    // EndSession will change the session online state, so force to refresh timeout
    // For all other on-line commands, LayerNTS will refresh timeout
    Controller::Instance().RefreshSessionTime();
    Controller::Instance().RefreshDispTime();
    Ack();
    // !!! first Ack, then set offline
    // 'cause nr&ns related to SESSION::ON_LINE
    session->Session(ISession::SESSION::OFF_LINE);
}

bool TsiSp003App::CheckOnline_RejectIfFalse()
{
    if (!IsOnline())
    {
        Reject(APP::ERROR::DeviceControllerOffline);
        return false;
    }
    return true;
}

bool TsiSp003App::IsOnline()
{
    if (session != nullptr)
    {
        return session->Session() == ISession::SESSION::ON_LINE;
    }
    return true;
}

void TsiSp003App::UpdateTime(uint8_t *data, int len)
{
    if (!CheckOnline_RejectIfFalse() || !ChkLen(len, 8))
        return;
    // set time
    time_t cur = time(nullptr);
    struct tm stm;
    data++;
    stm.tm_mday = *data++;
    stm.tm_mon = *data - 1;
    data++;
    stm.tm_year = Cnvt::GetU16(data) - 1900;
    data += 2;
    stm.tm_hour = *data++;
    stm.tm_min = *data++;
    stm.tm_sec = *data;
    stm.tm_isdst = -1;
    if (Time::IsTmValid(&stm))
    {
        time_t t = mktime(&stm);
        if (t > 0)
        {
            char buf[64];
            char *p = buf + sprintf(buf, "UpdateTime:");
            p = Cnvt::ParseTmToLocalStr(cur, p);
            sprintf(p, "->");
            Cnvt::ParseTmToLocalStr(t, p + 2);
            db.GetUciEvent().Push(0, buf);
            if (0)//stime(&t) < 0)
            {
                db.GetUciAlarm().Push(0, "UpdateTime failed, errno=%d", errno);
                db.GetUciFault().Push(0, DEV::ERROR::MemoryError, 1);
            }
            else
            {
                if (pDS3231->SetTimet(t) < 0)
                {
                    db.GetUciAlarm().Push(0, "UpdateTime: Set DS3231 time failed(MemoryError)");
                    db.GetUciFault().Push(0, DEV::ERROR::MemoryError, 1);
                }
            }
            Ack();
            return;
        }
    }
    SetRejectStr("Invalid time");
    Reject(APP::ERROR::SyntaxError);
}

uint16_t TsiSp003App::MakePassword()
{
    uint16_t passwd;
    uint8_t bit5, bit7, bit8;
    passwd = session->Seed();
    passwd += db.GetUciUser().SeedOffset();
    passwd = passwd & 0xFF;                  // set high byte to zero
    for (unsigned int ii = 0; ii < 16; ii++) // the process is cycled 16 times
    {
        bit5 = (passwd & 0x20) != 0;
        bit7 = (passwd & 0x80) != 0;
        bit8 = (passwd & 0x100) != 0;
        passwd <<= 1; // shift left one position
        passwd = passwd + (bit5 ^ bit7 ^ bit8);
    }
    return passwd + db.GetUciUser().PasswordOffset();
}

bool TsiSp003App::ChkLen(int rcvd, int expect)
{
    if (rcvd == expect)
    {
        return true;
    }
    SetRejectStr("Got[%d]Expect[%d]", rcvd, expect);
    Reject(APP::ERROR::LengthError);
    return false;
}
