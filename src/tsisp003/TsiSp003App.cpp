#include <tsisp003/TsiSp003App.h>
#include <sign/Scheduler.h>

TsiSp003App::TsiSp003App()
    : db(DbHelper::Instance()),
      sch(Scheduler::Instance()),
      session(nullptr)
{
}

TsiSp003App::~TsiSp003App()
{
}

int TsiSp003App::Rx(uint8_t *data, int len)
{
    micode = *data;
    switch (micode)
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
    default:
        return -1;
    }
    return 0;
}

void TsiSp003App::Clean()
{
    appErr = APP::ERROR::AppNoError;
}

void TsiSp003App::Reject(APP::ERROR error)
{
    uint8_t buf[3];
    buf[0] = MI::CODE::Reject;
    buf[1] = micode;
    buf[2] = error;
    Tx(buf, 3);
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

void TsiSp003App::Ack()
{
    uint8_t buf[2];
    buf[0] = MI::CODE::ACK;
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
    buf[0] = MI::CODE::PasswordSeed;
    buf[1] = seed;
    Tx(buf, 2);
}

void TsiSp003App::Password(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false || session == nullptr)
        return;
    if (session->Session() != ISession::SESSION::START)
    {
        Reject(APP::ERROR::SyntaxError);
        return;
    }
    uint16_t pass = data[1] * 0x100 + data[2];
    if (pass == MakePassword())
    {
        //SetStatusLed(1);
        Ack();
        // !!! first Ack, then set online
        session->Session(ISession::SESSION::ON_LINE);
    }
    else
    {
        session->Session(ISession::SESSION::OFF_LINE);
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
    if (!CheckOlineReject() || !ChkLen(len, 1) || session == nullptr)
        return;
    Ack();
    // !!! first Ack, then set offline
    session->Session(ISession::SESSION::OFF_LINE);
}

bool TsiSp003App::CheckOlineReject()
{
    if (!IsOnline())
    {
        Reject(APP::ERROR::DeviceControllerOffline);
        return false;
    }
    Scheduler::Instance().RefreshDispTime();
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
    if (!CheckOlineReject() || !ChkLen(len, 8))
        return;
    // set time
    Ack();
}

uint16_t TsiSp003App::MakePassword()
{
    uint16_t passwd;
    uint8_t bit5, bit7, bit8;
    passwd = session->Seed();
    passwd += DbHelper::Instance().uciUser.SeedOffset();
    passwd = passwd & 0xFF;                  // set high byte to zero
    for (unsigned int ii = 0; ii < 16; ii++) // the process is cycled 16 times
    {
        bit5 = (passwd & 0x20) != 0;
        bit7 = (passwd & 0x80) != 0;
        bit8 = (passwd & 0x100) != 0;
        passwd <<= 1; // shift left one position
        passwd = passwd + (bit5 ^ bit7 ^ bit8);
    }
    return passwd + DbHelper::Instance().uciUser.PasswordOffset();
}

bool TsiSp003App::ChkLen(int len1, int len2)
{
    if (len1 == len2)
    {
        return true;
    }
    Reject(APP::ERROR::LengthError);
    return false;
}
