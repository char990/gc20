#include <cstdio>
#include <cstring>
#include <uci.h>
#include <module/Utils.h>
#include <uci/UciMsg.h>
#include <module/MyDbg.h>

using namespace Utils;

UciMsg::UciMsg(UciFrm &uciFrm)
:uciFrm(uciFrm)
{
    PATH = "./config";
    PACKAGE = "UciMsg";
	SECTION = "msg";
}

UciMsg::~UciMsg()
{
}

void UciMsg::LoadConfig()
{
	chksum = 0;
	Open();
	struct uci_section *uciSec = GetSection(SECTION);
	struct uci_element *e;
	struct uci_option *option;

	Message  msg;
	uci_foreach_element(&uciSec->options, e)
	{
		if (memcmp(e->name, "msg_", 4) != 0)
			continue;
		struct uci_option *option = uci_to_option(e);
		int i = atoi(e->name + 4);
		if (i < 1 || i > 255 || option->type != uci_option_type::UCI_TYPE_STRING)
			continue;
		char *buf = option->v.string;
        APP::ERROR r = msg.Init(buf, strlen(buf));
		if ( r == APP::ERROR::AppNoError && msg.msgId == i && CheckMsgEntries(&msg) == 0 )
		{
            chksum-=msgs[i].crc;
            msgs[i] = msg;
            chksum+=msg.crc;
		}
	}
	Close();
	Dump();
}

int UciMsg::CheckMsgEntries(Message  * msg)
{
    int i=0;
    while(i<6)
    {
        uint8_t fid = msg->msgEntries[i].frmId;
        if(fid==0)
        {
            break;
        }
        else
        {
            if(uciFrm.GetFrm(fid)==nullptr)
            {
                return 1;
            }
        }
		i++;
    }
    return (i==0)?-1:0;
}

void UciMsg::Dump()
{
	for (int i = 1; i <= 255; i++)
	{
		if (msgs[i].micode != 0)
		{
			printf("%s\n", msgs[i].ToString().c_str());
		}
	}
}

uint16_t UciMsg::ChkSum()
{
	return chksum;
}

Message *UciMsg::GetMsg(int i)
{
	return &msgs[i];
}

uint8_t UciMsg::GetMsgRev(int i)
{
	return (i==0) ? 0 : msgs[i].msgRev;
}

APP::ERROR UciMsg::SetMsg(uint8_t *buf, int len)
{
	Message msg;
    APP::ERROR r = msg.Init(buf,len);
	if (r == APP::ERROR::AppNoError)
	{
		int i = msg.msgId;
      	chksum-=msgs[i].crc;
		msgs[i] = msg;
    	chksum+=msg.crc;
	}
	return r;
}

void UciMsg::SaveMsg(int i)
{
	if(i<1 || i>255 || msgs[i].micode==0)return;
    char option[8];
	sprintf(option,"msg_%d",i);
	uint8_t a[MSG_LEN_MAX];
	char v[(MSG_LEN_MAX+2)*2+1];
	int len = msgs[i].ToArray(a);
	char * p = Cnvt::ParseToAsc(a, v, len);
    p = Cnvt::ParseU16ToAsc(msgs[i].crc, p);
	*p='\0';
    Save(SECTION, option, v);
}
