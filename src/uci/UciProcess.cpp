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
}

UciProcess::~UciProcess()
{
	if (grpProc != nullptr)
	{
		delete[] grpProc;
	}
}

void UciProcess::LoadConfig()
{
	PATH = DbHelper::Instance().Path();
	PACKAGE = "UciProcess";
	SECTION = &GroupX[0];
	Open();
	DbHelper &db = DbHelper::Instance();
	grpCnt = db.GetUciProd().NumberOfGroups();
	grpProc = new GrpProc[grpCnt];
	char option[16];
	int buf[255];
	const char *str;
	int d;
	GrpProc *p;
	for (int i = 1; i <= grpCnt; i++)
	{
		sprintf(GroupX, "Group%d", i);
		struct uci_section *uciSec = GetSection(SECTION);
		p = GetGrpProc(i);
		str = GetStr(uciSec, _EnabledPlan);
		if (str != NULL)
		{
			int j = Cnvt::GetIntArray(str, 255, buf, 1, 255);
			for (int k = 0; k < j; k++)
			{
				if (db.GetUciPln().IsPlnDefined(buf[k]))
				{
					p->EnablePlan(buf[k]);
				}
			}
		}

		uint8_t *plen = p->ProcDisp();
		*plen = 0;
		str = GetStr(uciSec, _Display);
		if (str != NULL)
		{
			int len = strlen(str);
			if (len <= (255 * 2) && Cnvt::ParseToU8(str, p->ProcDisp() + 1, len) == 0)
			{
				*plen = len / 2;
			}
		}
		d = GetInt(uciSec, _Dimming, 0, 16);
		p->Dimming(d);
		d = GetInt(uciSec, _Power, 0, 1);
		p->Power(d);
		d = GetInt(uciSec, _Device, 0, 1);
		p->Device(d);
	}

	Close();
	Dump();
}

void UciProcess::Dump()
{
	PrintDash();
	printf("%s/%s\n", PATH, PACKAGE);
	char buf[1024];
	for (int i = 1; i <= grpCnt; i++)
	{
		auto p = GetGrpProc(i);
		PrintGrpPln(i, buf);
		printf("\tGroup%d.%s \t'%s'\n", i, _EnabledPlan, buf);
		uint8_t *disp = p->ProcDisp();
		if (*disp == 0)
		{
			printf("\tGroup%d.%s \t''\n", i, _Display);
		}
		else
		{
			Cnvt::ParseToStr(disp + 1, buf, *disp);
			printf("\tGroup%d.%s \t'%s'\n", i, _Display, buf);
		}
		printf("\tGroup%d.%s \t'%d'\n", i, _Dimming, p->Dimming());
		printf("\tGroup%d.%s \t'%d'\n", i, _Power, p->Power());
		printf("\tGroup%d.%s \t'%d'\n", i, _Device, p->Device());
	}
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

void UciProcess::EnablePlan(uint8_t gid, uint8_t pid)
{
	if (gid == 0 || gid > grpCnt)
		return;
	grpProc[gid - 1].EnablePlan(pid);
	SaveGrpPln(gid);
}

void UciProcess::DisablePlan(uint8_t gid, uint8_t pid)
{
	if (gid == 0 || gid > grpCnt)
		return;
	grpProc[gid - 1].DisablePlan(pid);
	SaveGrpPln(gid);
}

void UciProcess::SaveGrpPln(uint8_t gid)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(GroupX, "Group%d", gid);
	char buf[1024];
	PrintGrpPln(gid, buf);
	OpenSaveClose(SECTION, _EnabledPlan, buf);
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
	sprintf(GroupX, "Group%d", gid);

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
	sprintf(GroupX, "Group%d", gid);
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
	sprintf(GroupX, "Group%d", gid);
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
	sprintf(GroupX, "Group%d", gid);
	grpProc[gid - 1].Device(v);
	OpenSaveClose(SECTION, _Device, v);
}

uint8_t UciProcess::GetDevice(uint8_t gid)
{
	return (gid == 0 || gid > grpCnt) ? 0 : grpProc[gid - 1].Device();
}
