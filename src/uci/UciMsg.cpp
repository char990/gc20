#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <uci.h>
#include <module/Utils.h>
#include <uci/UciMsg.h>

using namespace Utils;

UciMsg::UciMsg(UciFrm &uciFrm)
:uciFrm(uciFrm)
{
	for(int i=0;i<=255;i++)
	{
		msgs[i] = nullptr;
	}
}

UciMsg::~UciMsg()
{
	for(int i=0;i<=255;i++)
	{
		if(msgs[i] != nullptr)
		{
			delete msgs[i];
		}
	}
}

void UciMsg::LoadConfig()
{
	chksum=0;
	chksum = 0;
	Open(PATH, PACKAGE);
	struct uci_section *section = uci_lookup_section(ctx, pkg, SECTION);
	if (section == NULL)
	{
		MyThrow("Can't load %s/%s.%s", PATH, PACKAGE, SECTION);
	}
	struct uci_element *e;
	struct uci_option *option;
	uci_foreach_element(&section->options, e)
	{
		if (memcmp(e->name, "msg_", 4) != 0)
			continue;
		struct uci_option *option = uci_to_option(e);
		int i = atoi(e->name + 4);
		if (i < 1 || i > 255 || option->type != uci_option_type::UCI_TYPE_STRING)
			continue;
		char *buf = option->v.string;
		int r = strlen(buf);
		if (r != 36)
			continue;
		Message  * msg = new Message(buf,36);
		if ( msg->appErr == APP::ERROR::AppNoError && msg->msgId == i && CheckMsgEntries(msg) == 0 )
		{
			if (msgs[i] != nullptr)
			{
				chksum -= msgs[i]->crc;
				delete msgs[i];
			}
			msgs[i]=msg;
			chksum += msgs[i]->crc;
		}
		else
		{
			delete msg;
		}
	}
	Close();
	Dump();
}

int UciMsg::CheckMsgEntries(Message  * msg)
{
    int r=-1;
    for(int i=0;i<6;i++)
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
            else
            {
                r=0;
            }
        }
    }
    return r;
}

void UciMsg::Dump()
{
	for (int i = 1; i < 255; i++)
	{
		if (msgs[i] != nullptr)
		{
			printf("%s\n", msgs[i]->ToString().c_str());
		}
	}
}

uint16_t UciMsg::ChkSum()
{
	return chksum;
}

Message *UciMsg::GetMsg(int i)
{
	return msgs[i];
}

uint8_t UciMsg::SetMsg(uint8_t *buf, int len)
{
	Message * msg = new Message(buf,len);
    uint8_t r = msg->appErr;
	if (r == APP::ERROR::AppNoError)
	{
		int i = msg->msgId;
		if (msgs[i] != nullptr)
		{
        	chksum-=msgs[i]->crc;
			delete msgs[i];
		}
		msgs[i] = msg;
    	chksum+=msgs[i]->crc;
	}
	else
	{
		delete msg;
	}
	return r;
}

void UciMsg::SaveMsg(int i)
{
	Message * msg = msgs[i];
	if(i<1 || i>255 || msg ==nullptr)return;
	Open(PATH, PACKAGE);
	char section[256];
	struct uci_ptr ptr;
	if(!GetPtr(&ptr, SECTION, section))
	{
		Close();
		return;
	}
	char option[8];
	sprintf(option,"msg_%d",i);
	char v[36+1];
	Cnvt::ParseToAsc(msg->msgData, v, msg->msgDatalen);
    v[msg->msgDatalen*2]='\0';
	ptr.option = option;
	ptr.value = v;
	uci_set(ctx, &ptr);
	Commit();
	Close();
}
