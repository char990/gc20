#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <uci.h>
#include <uci/UciFrm.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

UciFrm::UciFrm()
{
	for (int i = 0; i < 256; i++)
	{
		frms[i] = nullptr;
	}
}

UciFrm::~UciFrm()
{
	for (int i = 0; i < 256; i++)
	{
		if (frms[i] != nullptr)
		{
			delete frms[i];
		}
	}
}

void UciFrm::LoadConfig()
{
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
		if (memcmp(e->name, "frm_", 4) != 0)
			continue;
		struct uci_option *option = uci_to_option(e);
		int i = atoi(e->name + 4);
		if (i < 1 || i > 255 || option->type != uci_option_type::UCI_TYPE_STRING)
			continue;
		char *buf = option->v.string;
		int r = strlen(buf);
		if (r < 20)
			continue;
		int mi = Cnvt::ParseToU8(buf);
		Frame *frm;
		if (mi == MI::CODE::SignSetTextFrame)
		{
			frm = new FrmTxt(buf, r);
		}
		else if (mi == MI::CODE::SignSetGraphicsFrame)
		{
			frm = new FrmGfx(buf, r);
		}
		else if (mi == MI::CODE::SignSetHighResolutionGraphicsFrame)
		{
			frm = new FrmHrg(buf, r);
		}
		else
		{
			continue;
		}
		if (frm->appErr == APP::ERROR::AppNoError && frm->frmId == i)
		{
			chksum += frm->crc;
			if (frms[i] != nullptr)
			{
				delete frms[i];
			}
			frms[i] = frm;
		}
		else
		{
			delete frm;
		}
	}
	Close();
	Dump();
	//TestSaveTxtFrm();
}

void UciFrm::Dump()
{
	for (int i = 1; i <= 255; i++)
	{
		if (frms[i] != nullptr)
		{
			PrintDbg("%s\n", frms[i]->ToString().c_str());
		}
	}
}

uint16_t UciFrm::ChkSum()
{
	return chksum;
}

Frame *UciFrm::GetFrm(int i)
{
	return frms[i];
}

uint8_t UciFrm::SetFrm(uint8_t *buf, int len)
{
	Frame *frm;
	if (buf[0] == MI::CODE::SignSetTextFrame)
	{
		frm = new FrmTxt(buf, len);
	}
	else if (buf[0] == MI::CODE::SignSetGraphicsFrame)
	{
		frm = new FrmGfx(buf, len);
	}
	else if (buf[0] == MI::CODE::SignSetHighResolutionGraphicsFrame)
	{
		frm = new FrmHrg(buf, len);
	}
	else
	{
		return APP::ERROR::UnknownMi;
	}
	uint8_t r = frm->appErr;
	if (r == APP::ERROR::AppNoError)
	{
		int i = frm->frmId;
		if (frms[i] != nullptr)
		{
			chksum -= frms[i]->crc;
			delete frms[i];
		}
		frms[i] = frm;
		chksum += frms[i]->crc;
	}
	else
	{
		delete frm;
	}
	return r;
}

void UciFrm::SaveFrm(int i)
{
	if (i < 1 || i > 255)
		return;
	Open(PATH, PACKAGE);
	char section[256];
	struct uci_ptr ptr;
	SetPtr(&ptr, SECTION, section);
	char option[8];
	sprintf(option, "frm_%d", i);
	int len = (frms[i]->frmlen + 9);
	char *v = new char[len * 2 + 1];
	Cnvt::ParseToAsc(frms[i]->frmData, v, len);
	v[len * 2] = '\0';
	ptr.option = option;
	ptr.value = v;
	uci_set(ctx, &ptr);
	Commit();
	Close();
	delete v;
}

void UciFrm::TestSaveTxtFrm()
{
	PrintDbg("TestSaveTxtFrm\n");
#define TestSaveTxtFrm_len 10
	uint8_t buf[TestSaveTxtFrm_len + 9];
	buf[0] = MI::CODE::SignSetTextFrame;
	buf[3] = 0;
	buf[4] = 0;
	buf[5] = 0;
	buf[6] = TestSaveTxtFrm_len;
	for (int i = 0; i < TestSaveTxtFrm_len; i++)
	{
		buf[7 + i] = i + 'A';
	}
	for (int i = 1; i <= 255; i++)
	{
		buf[1] = i;
		buf[2] = 256 - i;
		uint16_t crc = Crc::Crc16_1021(buf, TestSaveTxtFrm_len + 7);
		buf[TestSaveTxtFrm_len + 7] = crc / 0x100;
		buf[TestSaveTxtFrm_len + 8] = crc & 0xFF;
		SetFrm(buf, TestSaveTxtFrm_len + 9);
	}
	PrintDbg("SetFrm complete\n");
	Dump();
#if 0
	for(int i=1;i<=255;i++)
	{
		SaveFrm(i);
	}
#else
	char option[8];
	int len = TestSaveTxtFrm_len + 9;
	char v[len * 2 + 1];
	Open(PATH, PACKAGE);
	char section[256];
	struct uci_ptr ptr;
	SetPtr(&ptr, SECTION, section);

	ptr.option = option;
	ptr.value = v;
	for (int i = 1; i <= 255; i++)
	{
		sprintf(option, "frm_%d", i);
		Cnvt::ParseToAsc(frms[i]->frmData, v, len);
		v[len * 2] = '\0';
		uci_set(ctx, &ptr);
	}
	
	Commit();
	Close();
#endif
}
