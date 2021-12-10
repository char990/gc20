#include <cstdio>
#include <cstring>
#include <uci.h>
#include <module/Utils.h>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

using namespace Utils;

UciMsg::UciMsg()
{
}

UciMsg::~UciMsg()
{
}

void UciMsg::LoadConfig()
{
	PrintDbg(DBG_LOG, ">>> Loading 'messages'\n");
	PATH = DbHelper::Instance().Path();
	PACKAGE = "UciMsg";
	SECTION = "msg";
	chksum = 0;
	Open();
	struct uci_section *uciSec = GetSection(SECTION);
	struct uci_element *e;
	struct uci_option *option;

	Message msg;
	uint8_t b[MSG_LEN_MAX + MSG_TAIL];
	int i;
	uci_foreach_element(&uciSec->options, e)
	{
		if (sscanf(e->name, _Option, &i) == 1)
		{
			option = uci_to_option(e);
			if (i >= 1 && i <= 255 && option->type == uci_option_type::UCI_TYPE_STRING)
			{
				int len = strlen(option->v.string);
				if (Cnvt::ParseToU8(option->v.string, b, len) == 0)
				{
					len /= 2;
					if (Crc::Crc16_1021(b, len - MSG_TAIL) == Cnvt::GetU16(b + len - MSG_TAIL))
					{
						SetMsg(b, len);
					}
				}
			}
		}
	}
	Close();
	//Dump();
}

void UciMsg::Dump()
{
	PrintDash('<');
	printf("%s/%s\n", PATH, PACKAGE);
	for (auto &m : msgs)
	{
		if (m.micode != 0)
		{
			printf("\t%s\n", m.ToString().c_str());
		}
	}
	PrintDash('>');
}

uint16_t UciMsg::ChkSum()
{
	return chksum;
}

bool UciMsg::IsMsgDefined(uint8_t i)
{
	return (i != 0 && msgs.at(i - 1).micode != 0);
}

Message *UciMsg::GetMsg(uint8_t i)
{
	return IsMsgDefined(i) ? &msgs.at(i - 1) : nullptr;
}

uint8_t UciMsg::GetMsgRev(uint8_t i)
{
	return IsMsgDefined(i) ? msgs.at(i - 1).msgRev : 0;
}

APP::ERROR UciMsg::SetMsg(uint8_t *buf, int len)
{
	Message msg;
	APP::ERROR r = msg.Init(buf, len);
	if (r == APP::ERROR::AppNoError)
	{
		int i = msg.msgId - 1;
		if (msgs.at(i).micode != 0)
		{
			chksum -= msgs.at(i).crc;
		}
		msgs.at(i) = msg;
		chksum += msg.crc;
	}
	return r;
}

void UciMsg::SaveMsg(uint8_t i)
{
	if (!IsMsgDefined(i))
		return;
	char option[16];
	sprintf(option, _Option, i);
	i--;
	uint8_t a[MSG_LEN_MAX + MSG_TAIL];
	int len = msgs.at(i).ToArray(a);
	char v[(MSG_LEN_MAX + MSG_TAIL) * 2 + 1];
	Cnvt::ParseToStr(a, v, len);
	OpenSaveClose(SECTION, option, v);
}

void UciMsg::Reset()
{
	for (auto &m : msgs)
	{
		m.micode = 0;
	}
	UciCfg::ClrSECTION();
}

bool UciMsg::IsMsgFlashing(uint8_t i)
{
	if (IsMsgDefined(i))
	{
		auto &msg = msgs.at(i - 1);
		auto &uciFrm = DbHelper::Instance().GetUciFrm();
		for (int k = 0; k < msg.entries; k++)
		{
			if (uciFrm.IsFrmFlashing(msg.msgEntries[k].frmId))
			{
				return true;
			}
		}
	}
	return false;
}
