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
	SECTION = &Groupx[0];
	grpProc=nullptr;
	grpCnt=0;
}

UciProcess::~UciProcess()
{
	if(grpProc!=nullptr)
	{
		delete [] grpProc;
	}
}

void UciProcess::LoadConfig()
{
    Open();
	DbHelper & db = DbHelper::Instance();
	grpCnt = db.uciUser.GroupCnt();
	grpProc = new GrpProc[grpCnt];
	char option[16];
	int pln[255];
	const char *str;
	for(int i=0;i<grpCnt;i++)
	{
		sprintf(Groupx,"Group%d",i+1);
	    struct uci_section *uciSec = GetSection(SECTION);
		
		str = GetStr(uciSec, _EnabledPlan);
		if(str!=NULL)
		{
			int j = Cnvt::GetIntArray(str, 255, pln, 1, 255);
			for(int k=0;k<j;k++)
			{
				if(db.uciPln.IsPlnDefined(pln[k]))
				{
					grpProc[i-1].EnablePlan(pln[k]);
				}
			}
		}
		
		int d = GetInt(uciSec, _Display, 0, 16);
		if( d!=DISP_STATUS::TYPE::FRM &&
			d!=DISP_STATUS::TYPE::MSG &&
			d!=DISP_STATUS::TYPE::PLN &&
			d!=DISP_STATUS::TYPE::ATF)
		{
			continue;
		}

		str = GetStr(uciSec, _FmpId);
		if(str!=NULL)
		{
			int j = Cnvt::GetIntArray(str, 16, pln, 0, 255);
			if( d==DISP_STATUS::TYPE::ATF)
			{
				if(j!=1) // signCnt check
				{
					continue;
				}
			}
			else
			{
				if(j!=1)
				{
					continue;
				}
				if(d==DISP_STATUS::TYPE::FRM)
				{
					if(DbHelper::Instance().uciFrm.GetFrm(pln[0])==nullptr)
					{
						continue;
					}
				}
				else 	if(d==DISP_STATUS::TYPE::MSG)
				{
					if(DbHelper::Instance().uciMsg.GetMsg(pln[0])==nullptr)
					{
						continue;
					}
				}
				else 	if(d==DISP_STATUS::TYPE::PLN)
				{
					d=static_cast<int>DISP_STATUS::TYPE::FRM;
					pln[0]=0;
				}
				auto procDisp = grpProc[i].ProcDisp();
				procDisp->dispType = static_cast<DISP_STATUS::TYPE>(d);
				procDisp->fmpLen=1;
				procDisp->fmpid[0]=pln[0];
			}
		}
	}

	Close();
	Dump();
}

void UciProcess::Dump()
{
	char buf[1024];
	for(int i=0;i<grpCnt;i++)
	{
		PrintGrpPln(i+1, buf);
		PrintDbg("Group%d.%s %s", i+1, _EnabledPlan, buf);
		PrintDbg("Group%d.%s %s", i+1, _Display, grpProc[i].DispType());
		PrintGrpFmpId(i+1, buf);
		PrintDbg("Group%d.%s %s", i+1, _FmpId, buf);
	}
}

GrpProc * UciProcess::GetGrpPln(uint8_t gid)
{
	return (gid==0 || gid>grpCnt) ? nullptr : &grpProc[gid-1];
}


bool UciProcess::IsPlanEnabled(uint8_t gid, uint8_t pid)
{
    if (gid==0 || gid>grpCnt) return false;
	return grpProc[gid-1].IsPlanEnabled(pid);
}

void UciProcess::EnablePlan(uint8_t gid, uint8_t pid)
{
    if (gid==0 || gid>grpCnt) return;
	grpProc[gid-1].EnablePlan(pid);
    SaveGrpPln(gid);
}

void UciProcess::DisablePlan(uint8_t gid, uint8_t pid)
{
    if (gid==0 || gid>grpCnt) return;
	grpProc[gid-1].DisablePlan(pid);
    SaveGrpPln(gid);
}

void UciProcess::SaveGrpPln(uint8_t gid)
{
    if (gid==0 || gid>grpCnt) return;
	sprintf(Groupx,"Group%d",gid);
	char buf[1024];
	PrintGrpPln(gid, buf);
	Save(SECTION, _EnabledPlan, buf);
}

int UciProcess::PrintGrpPln(uint8_t gid, char *buf)
{
    if (gid==0 || gid>grpCnt) return;
	gid--;
	int len = 0;
	for(int i=1;i<=255;i++)
	{
		if(grpProc[gid].IsPlanEnabled(i))
		{
			if(len>0)
			{
				buf[len++]=',';
			}
			len += sprintf(buf+len, "%d", i);
		}
	}
	return len;
}

void UciProcess::SetDisp(uint8_t gid, DISP_STATUS::TYPE dt, uint8_t * id, int len)
{
    if (gid==0 || gid>grpCnt) return;
	sprintf(Groupx,"Group%d",gid);
	OpenSectionForSave(SECTION);

	if(len>16)len=16;
	auto procDisp = grpProc[gid-1].ProcDisp();
	procDisp->dispType = dt;
	procDisp->fmpLen = len;
	memcpy(procDisp->fmpid, id, len);
	
	int x = static_cast<int>(dt);
	OptionSave(_Display, x);

	char buf[1024];
	PrintGrpFmpId(gid, buf);
	OptionSave(_FmpId, buf);

	CloseSectionForSave();
}

int UciProcess::PrintGrpFmpId(uint8_t gid, char *buf)
{
    if (gid==0 || gid>grpCnt) return;
	auto procDisp = grpProc[gid-1].ProcDisp();
	int len = 0;
	uint8_t *p=procDisp->fmpid;
	for(int i=0;i<procDisp->fmpLen;i++)
	{
		if(len>0)
		{
			buf[len++]=',';
		}
		len += sprintf(buf+len, "%d", *p++);
	}
	return len;
}
