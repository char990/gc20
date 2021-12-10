#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

UciProcess::UciProcess()
{
	grpProc = nullptr;
	grpCnt = 0;
	signErr = nullptr;
	signCnt = 0;
}

UciProcess::~UciProcess()
{
	if (grpProc != nullptr)
	{
		delete[] grpProc;
	}
	if (signErr != nullptr)
	{
		delete[] signErr;
	}
}

void UciProcess::LoadConfig()
{
	PrintDbg(DBG_LOG, ">>> Loading 'process'\n");
	PATH = DbHelper::Instance().Path();
	PACKAGE = "UciProcess";
	SECTION = &sectionBuf[0];
	Open();
	DbHelper &db = DbHelper::Instance();
	char option[16];
	int buf[255];
	const char *str;
	int d;
	struct uci_section *uciSec;

	/************************** GroupX *************************/
	grpCnt = db.GetUciProd().NumberOfGroups();
	grpProc = new GrpProc[grpCnt];
	GrpProc *p;
	for (int i = 1; i <= grpCnt; i++)
	{
		sprintf(sectionBuf, "%s%d", _Group, i);
		uciSec = GetSection(SECTION);
		p = GetGrpProc(i);
		// _EnabledPlan
		try
		{
			auto & enp = p->EnabledPln();
			ReadBits(uciSec, _EnabledPlan, enp);
			for (int k = 1; k <= 255; k++)
			{
				if (!db.GetUciPln().IsPlnDefined(k))
				{
					enp.ClrBit(k);
				}
			}
		}
		catch (...){};
		// _Display
		uint8_t *plen = p->ProcDisp();
		*plen = 0;
		str = GetStr(uciSec, _Display);
		if (str != NULL)
		{
			int len = strlen(str);
			if (len <= (255 * 2) && Cnvt::ParseToU8(str, plen + 1, len) == 0)
			{
				*plen = len / 2;
			}
		}
		// _Dimming, _Power, _Device
		d = GetInt(uciSec, _Dimming, 0, 16);
		p->Dimming(d);
		d = GetInt(uciSec, _Power, 0, 1);
		p->Power(d);
		d = GetInt(uciSec, _Device, 0, 1);
		p->Device(d);
	}

	/************************** Ctrller *************************/
	sprintf(sectionBuf, _Ctrller);
	uciSec = GetSection(SECTION);
	try
	{
		ReadBits(uciSec, _CtrllerError, ctrllerErr);
	}
	catch (...){};
	/************************** SignX *************************/
	signCnt = db.GetUciProd().NumberOfSigns();
	signErr = new Utils::Bits[signCnt];
	for (int i = 1; i <= signCnt; i++)
	{
		signErr[i - 1].Init(32);
		sprintf(sectionBuf, "%s%d", _Sign, i);
		uciSec = GetSection(SECTION);
		try
		{
			ReadBits(uciSec, _SignError, signErr[i - 1]);
		}
		catch (...){};
	}
	Close();
	Dump();
}

void UciProcess::Dump()
{
	PrintDash('<');
	printf("%s/%s\n", PATH, PACKAGE);
	char buf[1024];
	for (int i = 1; i <= grpCnt; i++)
	{
		printf("%s%d:\n", _Group, i);
		auto p = GetGrpProc(i);
		PrintGrpPln(i, buf);
		printf("\t%s \t'%s'\n", _EnabledPlan, buf);
		uint8_t *disp = p->ProcDisp();
		if (*disp == 0)
		{
			printf("\t%s \t''\n", _Display);
		}
		else
		{
			Cnvt::ParseToStr(disp + 1, buf, *disp);
			printf("\t%s \t'%s'\n", _Display, buf);
		}
		printf("\t%s \t'%d'\n", _Dimming, p->Dimming());
		printf("\t%s \t'%d'\n", _Power, p->Power());
		printf("\t%s \t'%d'\n", _Device, p->Device());
	}

	printf("%s:\n", _Ctrller);
	PrintOption_str(_CtrllerError, ctrllerErr.ToString().c_str());

	for (int i = 1; i <= signCnt; i++)
	{
		printf("%s%d:\n", _Sign, i);
		PrintOption_str(_SignError, signErr[i - 1].ToString().c_str());
	}
	PrintDash('>');
}

GrpProc *UciProcess::GetGrpProc(uint8_t gid)
{
	return (gid == 0 || gid > grpCnt) ? nullptr : &grpProc[gid - 1];
}

bool UciProcess::IsPlanEnabled(uint8_t gid, uint8_t pid)
{
	if (gid == 0 || gid > grpCnt)
		return false;
	return grpProc[gid - 1].IsPlanEnabled(pid);
}

void UciProcess::EnDisPlan(uint8_t gid, uint8_t pid, bool endis)
{
	if (gid == 0 || gid > grpCnt)
		return;
	grpProc[gid - 1].EnDisPlan(pid, endis);
	SaveGrpPln(gid);
}

void UciProcess::SaveGrpPln(uint8_t gid)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(sectionBuf, "%s%d", _Group, gid);
	//char * buf = new char[1024];
	char buf[1024];
	PrintGrpPln(gid, buf);
	OpenSaveClose(SECTION, _EnabledPlan, buf);
	//delete [] buf;
}

int UciProcess::PrintGrpPln(uint8_t gid, char *buf)
{
	if (gid == 0 || gid > grpCnt)
		return 0;
	buf[0] = '\0';
	auto p = GetGrpProc(gid);
	int len = 0;
	for (int i = 1; i <= 255; i++)
	{
		if (p->IsPlanEnabled(i))
		{
			if (len > 0)
			{
				buf[len++] = ',';
			}
			len += sprintf(buf + len, "%d", i);
		}
	}
	if (len == 0)
	{
		len = 1;
		buf[0] = ' ';
		buf[1] = '\0';
	}
	return len;
}

uint8_t *UciProcess::GetDisp(uint8_t gid)
{
	return (gid == 0 || gid > grpCnt) ? nullptr : grpProc[gid - 1].ProcDisp();
}

void UciProcess::SetDisp(uint8_t gid, uint8_t *cmd, int len)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(sectionBuf, "%s%d", _Group, gid);

	grpProc[gid - 1].ProcDisp(cmd, len);

	uint8_t *p = grpProc[gid - 1].ProcDisp();
	len = *p;
	char v[255 * 2 + 1];
	if (len == 0)
	{
		sprintf(v, " ");
	}
	else
	{
		Cnvt::ParseToStr(p + 1, v, len);
	}
	OpenSaveClose(SECTION, _Display, v);
}

void UciProcess::SetDimming(uint8_t gid, uint8_t v)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(sectionBuf, "%s%d", _Group, gid);
	grpProc[gid - 1].Dimming(v);
	OpenSaveClose(SECTION, _Dimming, v);
}

uint8_t UciProcess::GetDimming(uint8_t gid)
{
	return (gid == 0 || gid > grpCnt) ? 0 : grpProc[gid - 1].Dimming();
}

void UciProcess::SetPower(uint8_t gid, uint8_t v)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(sectionBuf, "%s%d", _Group, gid);
	grpProc[gid - 1].Power(v);
	OpenSaveClose(SECTION, _Power, v);
}

uint8_t UciProcess::GetPower(uint8_t gid)
{
	return (gid == 0 || gid > grpCnt) ? 0 : grpProc[gid - 1].Power();
}

void UciProcess::SetDevice(uint8_t gid, uint8_t v)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(sectionBuf, "%s%d", _Group, gid);
	grpProc[gid - 1].Device(v);
	OpenSaveClose(SECTION, _Device, v);
}

uint8_t UciProcess::GetDevice(uint8_t gid)
{
	return (gid == 0 || gid > grpCnt) ? 0 : grpProc[gid - 1].Device();
}

void UciProcess::SaveCtrllerErr(Utils::Bits &v)
{
	ctrllerErr.Clone(v);
	OpenSaveClose(_Ctrller, _CtrllerError, ctrllerErr);
}

void UciProcess::SaveSignErr(uint8_t signId, Utils::Bits &v)
{
	if (signId == 0 || signId > signCnt)
		return;
	signErr[signId - 1].Clone(v);
	sprintf(sectionBuf, "%s%d", _Sign, signId);
	OpenSaveClose(SECTION, _SignError, signErr[signId - 1]);
}
