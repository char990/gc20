#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/UciProcess.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

UciProcess::UciProcess()
{
	PATH = "./config";
	PACKAGE = "UciProcess";
	SECTION = &GroupX[0];
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
	Open();
	DbHelper &db = DbHelper::Instance();
	grpCnt = db.uciProd.NumberOfGroups();
	grpProc = new GrpProc[grpCnt];
	char option[16];
	int pln[255];
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
			int j = Cnvt::GetIntArray(str, 255, pln, 1, 255);
			for (int k = 0; k < j; k++)
			{
				if (db.uciPln.IsPlnDefined(pln[k]))
				{
					p->EnablePlan(pln[k]);
				}
			}
		}

		d = GetInt(uciSec, _Dimming, 0, 16);
		p->Dimming(d);

		d = GetInt(uciSec, _Display, 0, 16);
		if (d != DISP_STATUS::TYPE::N_A &&
			d != DISP_STATUS::TYPE::FRM &&
			d != DISP_STATUS::TYPE::MSG &&
			d != DISP_STATUS::TYPE::PLN &&
			d != DISP_STATUS::TYPE::ATF)
		{
			continue;
		}

		str = GetStr(uciSec, _FmpId);
		if (str != NULL)
		{
			int j = Cnvt::GetIntArray(str, 16, pln, 0, 255);
			if (d == DISP_STATUS::TYPE::ATF)
			{
				if (j != 1) // signCnt check
				{
					continue;
				}
			}
			else
			{
				if (j != 1)
				{
					continue;
				}
				if (d == DISP_STATUS::TYPE::FRM)
				{
					if (pln[0] != 0 && !DbHelper::Instance().uciFrm.IsFrmDefined(pln[0]))
					{
						continue;
					}
				}
				else if (d == DISP_STATUS::TYPE::MSG)
				{
					if (pln[0] != 0 && !DbHelper::Instance().uciMsg.IsMsgDefined(pln[0]))
					{
						continue;
					}
				}
				else if (d == DISP_STATUS::TYPE::PLN)
				{
					d = static_cast<int>(DISP_STATUS::TYPE::FRM);
					pln[0] = 0;
				}
				auto procDisp = p->ProcDisp();
				procDisp->dispType = static_cast<DISP_STATUS::TYPE>(d);
				procDisp->fmpLen = 1;
				procDisp->fmpid[0] = pln[0];
			}
		}
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
		printf("\tGroup%d.%s \t'%d'\n", i, _Display, p->ProcDisp()->dispType);
		PrintGrpFmpId(i, buf);
		printf("\tGroup%d.%s \t'%s'\n", i, _FmpId, buf);
		printf("\tGroup%d.%s \t'%d'\n", i, _Dimming, p->Dimming());
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
	buf[0]='\0';
	auto p =GetGrpProc(gid);
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
	if(len==0)
	{
		len=1;
		buf[0]=' ';
		buf[1]='\0';
	}
	return len;
}

void UciProcess::SetDisp(uint8_t gid, GrpProcDisp * disp)
{
	if (gid == 0 || gid > grpCnt)
		return;
	sprintf(GroupX, "Group%d", gid);
	OpenSectionForSave(SECTION);

	memcpy(grpProc[gid - 1].ProcDisp(), disp, sizeof(GrpProcDisp));

	int x = static_cast<int>(disp->dispType);
	OptionSave(_Display, x);

	char buf[1024];
	PrintGrpFmpId(gid, buf);
	OptionSave(_FmpId, buf);

	CloseSectionForSave();
}

GrpProcDisp * UciProcess::GetDisp(uint8_t gid) 
{
	return (gid == 0 || gid > grpCnt) ? nullptr : grpProc[gid - 1].ProcDisp();
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

int UciProcess::PrintGrpFmpId(uint8_t gid, char *buf)
{
	if (gid == 0 || gid > grpCnt)
		return 0;
	buf[0]='\0';
	auto procDisp = grpProc[gid - 1].ProcDisp();
	int len = 0;
	uint8_t *p = procDisp->fmpid;
	for (int i = 0; i < procDisp->fmpLen; i++)
	{
		if (len > 0)
		{
			buf[len++] = ',';
		}
		len += sprintf(buf + len, "%d", *p++);
	}
	return len;
}
