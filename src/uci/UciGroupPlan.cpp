#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/UciGroupPlan.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

UciGroupPlan::UciGroupPlan()
{
	PATH = "./config";
	PACKAGE = "UciGroupPlan";
	SECTION = "gpn";
	grpPln=nullptr;
	grpCnt=0;
}

UciGroupPlan::~UciGroupPlan()
{
	if(grpPln!=nullptr)
	{
		delete [] grpPln;
	}
}

void UciGroupPlan::LoadConfig()
{
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
	DbHelper & db = DbHelper::Instance();
	grpCnt = db.uciUser.GroupCnt();
	grpPln = new GrpPln[grpCnt];
	char option[16];
	int pln[255];
    for(int i=1;i<grpCnt+1;i++)
	{
		sprintf(option, "Group%d", i);
		const char *str = GetStr(uciSec, option);
		if(str!=NULL)
		{
			GrpPln & gp = grpPln[i-1];
			int j = Cnvt::GetIntArray(str, 255, pln, 1, 255);
			for(int k=0;k<j;k++)
			{
				if(db.uciPln.IsPlnDefined(pln[k]))
				{
					gp.EnablePlan(pln[k]);
				}
			}
		}
	}
	Close();
	Dump();
}

void UciGroupPlan::Dump()
{
	char buf[1024];
	for(int i=0;i<grpCnt;i++)
	{
		PrintGrpPln(i+1, buf);
		PrintDbg("Group%d: %s", i+1, buf);
	}
}

GrpPln * UciGroupPlan::GetGrpPln(uint8_t gid)
{
	return (gid==0 || gid>grpCnt) ? nullptr : &grpPln[gid-1];
}


bool UciGroupPlan::IsPlanEnabled(uint8_t gid, uint8_t pid)
{
    if (gid==0 || gid>grpCnt) return false;
	return grpPln[gid-1].IsPlanEnabled(pid);
}

void UciGroupPlan::EnablePlan(uint8_t gid, uint8_t pid)
{
    if (gid==0 || gid>grpCnt) return;
	grpPln[gid-1].EnablePlan(pid);
    SaveGrpPln(gid);
}

void UciGroupPlan::DisablePlan(uint8_t gid, uint8_t pid)
{
    if (gid==0 || gid>grpCnt) return;
	grpPln[gid-1].DisablePlan(pid);
    SaveGrpPln(gid);
}

void UciGroupPlan::SaveGrpPln(uint8_t gid)
{
	if(gid==0) return;
	char option[16];
	sprintf(option, "Group%d", gid);
	char buf[1024];
	PrintGrpPln(gid, buf);
	Save(SECTION, option, buf);
}

int UciGroupPlan::PrintGrpPln(uint8_t gid, char *buf)
{
	if(gid==0) return 0;
	gid--;
	int len = 0;
	for(int i=1;i<=255;i++)
	{
		if(grpPln[gid].IsPlanEnabled(i))
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