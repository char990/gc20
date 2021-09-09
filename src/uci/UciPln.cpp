#include <uci/UciPln.h>
#include <module/Utils.h>

using namespace Utils;

UciPln::UciPln(UciFrm &uciFrm, UciMsg &uciMsg) 
:uciFrm(uciFrm), uciMsg(uciMsg)
{
    PATH = "./config";
    PACKAGE = "UciPln";
    SECTION = "pln";
    for(int i=0;i<=255;i++)
    {
        plns[i]=nullptr;
    }
}

UciPln::~UciPln() 
{
    for(int i=0;i<=255;i++)
    {
        if(plns[i]!=nullptr)
        {
            delete plns[i];
        }
    }
}

void UciPln::LoadConfig()
{

    Dump();
}

void UciPln::Dump()
{
	for (int i = 1; i <= 255; i++)
	{
		if (plns[i] != nullptr)
		{
			printf("%s\n", plns[i]->ToString().c_str());
		}
	}
}

uint16_t UciPln::ChkSum()
{
    return chksum;
}

Plan * UciPln::GetPln(int i) 
{
    return plns[i];
}

uint8_t UciPln::GetPlnRev(int i)
{
	return (i==0) ? 0 : plns[i]->plnRev;
}

uint8_t UciPln::SetPln(uint8_t * buf, int len) 
{
	Plan * pln = new Plan(buf,len);
    uint8_t r = pln->appErr;
	if (r == APP::ERROR::AppNoError)
	{
		int i = pln->plnId;
		if (plns[i] != nullptr)
		{
        	chksum-=plns[i]->crc;
			delete plns[i];
		}
		plns[i] = pln;
    	chksum+=plns[i]->crc;
	}
	else
	{
		delete pln;
	}
	return r;
}

void UciPln::SavePln(int i) 
{
	Plan * pln = plns[i];
	if(i<1 || i>255 || pln ==nullptr)return;
	char option[8];
	sprintf(option,"pln_%d",i);
	char v[36+1];
	Cnvt::ParseToAsc(pln->plnData, v, pln->plnDataLen);
    v[pln->plnDataLen*2]='\0';
    Save(SECTION, option, v);
}

int UciPln::CheckPlnEntries(Plan  * pln) 
{
    int i=0;
    while(i<6)
    {
        if(pln->plnEntries[i].type==0)
        {
            break;
        }
        else if(pln->plnEntries[i].type==1)
        {
            if(uciFrm.GetFrm(pln->plnEntries[i].fmId)==nullptr)
            {
                return 1;
            }
        }
        else if(pln->plnEntries[i].type==2)
        {
            if(uciMsg.GetMsg(pln->plnEntries[i].fmId)==nullptr)
            {
                return 1;
            }
        }
        else
        {
            return -1;
        }
        i++;
    }
    return (i==0)?-1:0;
}


