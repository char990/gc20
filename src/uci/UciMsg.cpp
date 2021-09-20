#include <cstdio>
#include <cstring>
#include <uci.h>
#include <module/Utils.h>
#include <uci/UciMsg.h>
#include <module/MyDbg.h>

using namespace Utils;

UciMsg::UciMsg()
{
	PATH = "./config";
	PACKAGE = "UciMsg";
	SECTION = "msg";
	msgs = new Message[255];
}

UciMsg::~UciMsg()
{
	delete[] msgs;
}

void UciMsg::LoadConfig()
{
	chksum = 0;
	Open();
	struct uci_section *uciSec = GetSection(SECTION);
	struct uci_element *e;
	struct uci_option *option;

	Message msg;
	uint8_t b[MSG_LEN_MAX + MSG_TAIL];
	uci_foreach_element(&uciSec->options, e)
	{
		if (memcmp(e->name, "msg_", 4) != 0)
			continue;
		struct uci_option *option = uci_to_option(e);
		int i = atoi(e->name + 4);
		if (i < 1 || i > 255 || option->type != uci_option_type::UCI_TYPE_STRING)
			continue;
		int len = strlen(option->v.string);
		if (Cnvt::ParseToU8(option->v.string, b, len) == 0)
		{
			len/=2;
			if (Crc::Crc16_1021(b, len - MSG_TAIL) == Cnvt::GetU16(b + len - MSG_TAIL))
			{
				SetMsg(b, len);
			}
		}
	}
	Close();
	Dump();
}

void UciMsg::Dump()
{
	for (int i = 1; i <= 255; i++)
	{
		if (IsMsgDefined(i))
		{
			PrintDbg("%s", msgs[i - 1].ToString().c_str());
		}
	}
}

uint16_t UciMsg::ChkSum()
{
	return chksum;
}

bool IsMsgDefined(uint8_t i)
{
	return (i != 0 && msgs[i - 1].micode != 0);
}

Message *UciMsg::GetMsg(uint8_t i)
{
	return  IsMsgDefined(i) ? nullptr : &msgs[i - 1];
}

uint8_t UciMsg::GetMsgRev(uint8_t i)
{
	return IsMsgDefined(i) ? 0 : msgs[i - 1].msgRev;
}

APP::ERROR UciMsg::SetMsg(uint8_t *buf, int len)
{
	Message msg;
	APP::ERROR r = msg.Init(buf, len);
	if (r == APP::ERROR::AppNoError)
	{
		int i = msg.msgId - 1;
		if(msgs[i].micode !=0)
		{
			chksum -= msgs[i].crc;
		}
		msgs[i] = msg;
		chksum += msg.crc;
	}
	return r;
}

void UciMsg::SaveMsg(uint8_t i)
{
	if (!IsMsgDefined(i))
		return;
	char option[8];
	sprintf(option, "msg_%d", i);
	i--;
	uint8_t a[MSG_LEN_MAX + MSG_TAIL];
	int len = msgs[i].ToArray(a);
	char v[(MSG_LEN_MAX + MSG_TAIL) * 2 + 1];
	Cnvt::ParseToStr(a, v, len);
	Save(SECTION, option, v);
}
