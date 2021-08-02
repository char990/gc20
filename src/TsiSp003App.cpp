#include "TsiSp003App.h"
#include "Controller.h"

TsiSp003App::TsiSp003App()
:db(DbHelper::Instance())
 ctrl(Controller::Instance())
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
    case MI_CODE::StartSession:
        StartSession(data, len);
        break;
    case MI_CODE::Password:
        Password(data, len);
        break;
    case MI_CODE::EndSession:
        EndSession(data, len);
        break;
    case MI_CODE::SignSetDimmingLevel:
        SignSetDimmingLevel(data, len);
        break;
    case MI_CODE::PowerONOFF:
        PowerONOFF(data, len);
        break;
    case MI_CODE::DisableEnableDevice:
        DisableEnableDevice(data, len);
        break;
    case MI_CODE::RetrieveFaultLog:
        RetrieveFaultLog(data, len);
        break;
    case MI_CODE::ResetFaultLog:
        ResetFaultLog(data, len);
        break;
    default:
        return -1;
    }
    return 0;
}

void TsiSp003App::Clean()
{
    online=false;
    startSession=0;
    appError = APP_ERROR::AppNoError;
}

void TsiSp003App::Reject(uint8_t error)
{
    uint8_t buf[3];
    buf[0] = MI_CODE::Reject;
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
    buf[0] = MI_CODE::ACK;
    buf[1] = micode;
    Tx(buf, 2);
}

void TsiSp003App::StartSession(uint8_t *data, int len)
{
    if (ChkLen(len, 1) == false)
        return;
    startSession = 1;
    online = false;
    seed = rand();
    uint8_t buf[2];
    buf[0] = MI_CODE::PasswordSeed;
    buf[1] = seed;
    Tx(buf, 2);
}

void TsiSp003App::Password(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    if (startSession == 0)
    {
        Reject(APP_ERROR::SyntaxError);
        return;
    }
    uint16_t pass = data[1] * 0x100 + data[2];
    if (pass == MakePassword())
    {
        //SetStatusLed(1);
        startSession = 0;
        online = true;
        Ack();
    }
    else
    {
        startSession = 0;
        Reject(APP_ERROR::SyntaxError);
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
    if (ChkLen(len, 1) == false)
        return;
    startSession = 0;
    online = false;
    Ack();
}

///  ???
void TsiSp003App::UpdateTime(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003App::SignSetDimmingLevel(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003App::PowerONOFF(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003App::DisableEnableDevice(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003App::RetrieveFaultLog(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

void TsiSp003App::ResetFaultLog(uint8_t *data, int len)
{
    if (ChkLen(len, 3) == false)
        return;
    Reject(APP_ERROR::SyntaxError);
    Ack();
}

uint16_t TsiSp003App::MakePassword()
{
    uint16_t passwd;
    uint8_t bit5, bit7, bit8;
    passwd = (uint16_t)seed + DbHelper::Instance().SeedOffset();
    passwd = passwd & 0xFF;                  // set high byte to zero
    for (unsigned int ii = 0; ii < 16; ii++) // the process is cycled 16 times
    {
        bit5 = (passwd & 0x20) != 0;
        bit7 = (passwd & 0x80) != 0;
        bit8 = (passwd & 0x100) != 0;
        passwd <<= 1; // shift left one position
        passwd = passwd + (bit5 ^ bit7 ^ bit8);
    }
    return passwd + DbHelper::Instance().PasswdOffset();
}

bool TsiSp003App::ChkLen(int len1, int len2)
{
    if (len1 == len2)
    {
        return true;
    }
    Reject(APP_ERROR::LengthError);
    return false;
}
