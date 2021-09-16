#include <cstdio>
#include <cstring>
#include <ctime>

#include <uci.h>
#include <module/Utils.h>
#include <uci/UciStrLog.h>
#include <module/MyDbg.h>

using namespace Utils;

void UciStrLog::LoadConfig()
{
    for(int i=0;i<maxEntries;i++)
    {
        (pStrLog+i)->logTime=-1;
    }
	Open();
	struct uci_section *uciSec = GetSection(SECTION);
	struct uci_element *e;
	struct uci_option *option;
    char option_[8];
    int optLen = snprintf(option_, 7, "%s_",SECTION);
    char *pc;
	uci_foreach_element(&uciSec->options, e)
	{
		if (memcmp(e->name, option_, optLen) != 0)
		{
            continue;
        }
		struct uci_option *option = uci_to_option(e);
		int i = atoi(e->name + optLen);
		if (i < 0 || i >= maxEntries || option->type != uci_option_type::UCI_TYPE_STRING)
		{
            continue;
        }
        pc = option->v.string;
        (pStrLog+i)->id=Cnvt::ParseToU8(pc);pc++;
        (pStrLog+i)->entryNo=Cnvt::ParseToU16(pc);pc+=2;
        if( *pc++ !='[' )
        {
            continue;
        }
        (pStrLog+i)->logTime=Cnvt::ParseLocalStrToTm(pc);
        if( (pStrLog+i)->logTime <= 0 )
        {
            continue;
        }
        pc=strchr( pc, ']' );
        if(pc++==NULL)
        {
            (pStrLog+i)->logTime=-1;
            continue;
        }
        strncpy( (pStrLog+i)->str, pc, STR_SIZE-1 );
        (pStrLog+i)->str[STR_SIZE]='\0';
	}

    try
    {
        lastLog = GetInt(uciSec, _LastLog, 0, maxEntries-1);
    }
    catch(...)
    {
        time_t t=0;
        for(int i=0;i<maxEntries;i++)
        {
            if((pStrLog+i)->logTime>t)
            {
                lastLog = i;
                t=(pStrLog+i)->logTime;
            }
        }
    }

	Close();
}

int UciStrLog::GetLog(uint8_t *dst)
{
	if(lastLog<0)
    {
        return 0;
    }
    char *c;
    uint8_t *p=dst+2;
    int cnt=0;
    int log=lastLog;
    for(int i=0;i<maxEntries;i++)
    {
        if((pStrLog+log)->logTime>=0)
        {
            *p++=(pStrLog+log)->id;
            p=Cnvt::PutU16((pStrLog+log)->entryNo, p);
            p=Cnvt::PutLocalTm((pStrLog+log)->logTime, p);
            c=(pStrLog+log)->str;
            for(int j=0;j<STR_SIZE-1;j++)
            {
                if(*c=='\0')
                {
                    break;
                }
                *p++=*c++;
            }
            *p++='\0';
            cnt++;
            if(--log<0)
            {
                log = maxEntries-1;
            }
        }
    }
    dst[0]=cnt/0x100;
    dst[1]=cnt&0xFF;
    return p-dst;
}

void UciStrLog::Push(uint8_t id, const char *pbuf)
{
    uint16_t entryNo=0;
    if(lastLog!=-1)
    {
        entryNo = (pStrLog+lastLog)->entryNo+1;
    }
    lastLog++;
    if(lastLog>=maxEntries)
    {
        lastLog=0;
    }

    (pStrLog+lastLog)->id = id;
    (pStrLog+lastLog)->entryNo = entryNo;
    time_t t=time(NULL);
    (pStrLog+lastLog)->logTime = t;
    strncpy((pStrLog+lastLog)->str, pbuf, STR_SIZE-1);
    char v[(1+2)*2 + 21 + STR_SIZE + 1];
    char *p;
    p=Cnvt::ParseToAsc(id,v);
    p=Cnvt::ParseU16ToAsc(entryNo,p);
    *p++='[';
    p=Cnvt::ParseTmToLocalStr(t, p);
    *p++=']';
  	strncpy(p,pbuf,STR_SIZE-1);
    v[(1+2)*2 + 21 + STR_SIZE] = '\0';
    SaveLastLog(v);
}
