#include <cstdio>
#include <cstring>
#include <uci.h>
#include <module/Utils.h>
#include <uci/UciPln.h>
#include <module/MyDbg.h>

using namespace Utils;

UciPln::UciPln(UciFrm &uciFrm, UciMsg &uciMsg) 
:uciFrm(uciFrm), uciMsg(uciMsg)
{
    PATH = "./config";
    PACKAGE = "UciPln";
    SECTION = "pln";
}

UciPln::~UciPln() 
{
}

void UciPln::LoadConfig()
{
	chksum = 0;
	Open();
	struct uci_section *uciSec = GetSection(SECTION);
	struct uci_element *e;
	struct uci_option *option;
    Plan pln;
	uci_foreach_element(&uciSec->options, e)
	{
		if (memcmp(e->name, "pln_", 4) != 0)
			continue;
		struct uci_option *option = uci_to_option(e);
		int i = atoi(e->name + 4);
		if (i < 1 || i > 255 || option->type != uci_option_type::UCI_TYPE_STRING)
			continue;
		char *buf = option->v.string;
        APP::ERROR r = pln.Init(buf, strlen(buf));
		if ( r == APP::ERROR::AppNoError && pln.plnId == i && CheckPlnEntries(&pln) == 0 )
		{
            chksum-=plns[i].crc;
            plns[i] = pln;
            chksum+=pln.crc;
		}
	}
	Close();
	Dump();
}

void UciPln::Dump()
{
	for (int i = 1; i <= 255; i++)
	{
		if (plns[i].micode != 0)
		{
			printf("%s\n", plns[i].ToString().c_str());
		}
	}
}

uint16_t UciPln::ChkSum()
{
    return chksum;
}

Plan * UciPln::GetPln(int i) 
{
    return &plns[i];
}

uint8_t UciPln::GetPlnRev(int i)
{
	return (i==0) ? 0 : plns[i].plnRev;
}

APP::ERROR UciPln::SetPln(uint8_t * buf, int len)
{
	Plan pln;
    pln.enabled = PLN_DISABLED;
    APP::ERROR r = pln.Init(buf,len);
	if (r == APP::ERROR::AppNoError)
	{
		int i = pln.plnId;
      	chksum-=plns[i].crc;
		plns[i] = pln;
    	chksum+=pln.crc;
	}
	return r;
}

void UciPln::SavePln(int i) 
{
	if(i<1 || i>255 || plns[i].micode==0)return;
    char option[8];
	sprintf(option,"pln_%d",i);
	uint8_t a[PLN_LEN_MAX];
	char v[(PLN_LEN_MAX+2+1)*2+1];
	int len = plns[i].ToArray(a);
	char * p = Cnvt::ParseToAsc(a, v, len);
    p = Cnvt::ParseU16ToAsc(plns[i].crc, p);
	p = Cnvt::ParseToAsc(plns[i].enabled, p);
    *p='\0';
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
            if(uciMsg.GetMsg(pln->plnEntries[i].fmId)->micode==0)
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


