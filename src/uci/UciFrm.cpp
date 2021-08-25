#include <unistd.h>
#include <fcntl.h>
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
    PATH = "./config";
    PACKAGE = "UciFrm";
    SECTION = "frm";
	for (int i = 0; i <= 255; i++)
	{
		frms[i] = nullptr;
	}
}

UciFrm::~UciFrm()
{
	for (int i = 0; i <= 255; i++)
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
	Open();
	struct uci_section *sec = GetSection(SECTION.c_str());
	struct uci_element *e;
	struct uci_option *option;
	char *fbuf = new char [MAX_HRGFRM_SIZE*2];
	uci_foreach_element(&sec->options, e)
	{
		if (memcmp(e->name, "frm_", 4) != 0)
			continue;
		option = uci_to_option(e);
		int i = atoi(e->name + 4);
		if (i < 1 || i > 255 || option->type != uci_option_type::UCI_TYPE_STRING)
			continue;
		char *buf = option->v.string;
		int r = strlen(buf);
		int mi = Cnvt::ParseToU8(buf);
		Frame *frm;
		if (mi == MI::CODE::SignSetTextFrame)
		{
			if (r < MIN_TXTFRM_SIZE*2 || r > MAX_TXTFRM_SIZE*2)
				continue;
			frm = new FrmTxt(buf, r);
		}
		else if (mi == MI::CODE::SignSetGraphicsFrame)
		{
			if (r != (GFXFRM_HEADER_SIZE+2)*2)
				continue;
			int frmlen = Cnvt::ParseToU16(&buf[7*2]);
			if (frmlen<1 || frmlen>(MAX_GFXFRM_SIZE-GFXFRM_HEADER_SIZE-2))
				continue;
			memcpy(fbuf, buf, GFXFRM_HEADER_SIZE*2);					// copy frame header
			memcpy(fbuf+(GFXFRM_HEADER_SIZE+frmlen)*2, buf+r-2*2, 2*2);// copy frame crc
			char filename[256];
			snprintf(filename, 255, "%s/frm_%d", PATH, i);
			int frm_fd = open(filename, O_RDONLY);
			if(frm_fd>0)
			{
				int rdlen = read(frm_fd, fbuf+GFXFRM_HEADER_SIZE*2, frmlen * 2);
				close(frm_fd);
				if(rdlen!=frmlen*2)
				{
					continue;
				}
			}
			r+=frmlen*2;
			frm = new FrmGfx(buf, r);
		}
		else if (mi == MI::CODE::SignSetHighResolutionGraphicsFrame)
		{
			if (r != (HRGFRM_HEADER_SIZE+2)*2)
				continue;
			int64_t x = Cnvt::ParseToU32(&buf[9*2]);
			if(x<0)
				continue;
			int frmlen = x;
			if (frmlen<1 || frmlen>(MAX_HRGFRM_SIZE-HRGFRM_HEADER_SIZE-2))
				continue;
			memcpy(fbuf, buf, HRGFRM_HEADER_SIZE*2);					// copy frame header
			memcpy(fbuf+(HRGFRM_HEADER_SIZE+frmlen)*2, buf+r-2*2, 2*2);// copy frame crc
			char filename[256];
			snprintf(filename, 255, "%s/frm_%d", PATH, i);
			int frm_fd = open(filename, O_RDONLY);
			if(frm_fd>0)
			{
				int rdlen = read(frm_fd, fbuf+HRGFRM_HEADER_SIZE*2, frmlen * 2);
				close(frm_fd);
				if(rdlen!=frmlen*2)
				{
					continue;
				}
			}
			r+=frmlen*2;
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
	if (i < 1 || i > 255 || frms[i]==nullptr)
		return;

	int len;
	char *v;
	if(frms[i]->micode==MI::CODE::SignSetTextFrame)
	{
		// make option.value
		len = (frms[i]->frmlen + frms[i]->frmOffset + 2);
		v = new char[len * 2 + 1];
		Cnvt::ParseToAsc(frms[i]->frmData, v, len);
		v[len * 2] = '\0';
	}
	else
	{
		// save bitmap
		char * bitmap = new char[frms[i]->frmlen*2];
		Cnvt::ParseToAsc(frms[i]->frmData+frms[i]->frmOffset, bitmap, frms[i]->frmlen);
		char filename[256];
		snprintf(filename, 255, "%s/frm_%d", PATH.c_str(), i);
		int frm_fd = open(filename, O_CREAT|O_WRONLY|O_TRUNC, 0770);
		if(frm_fd>0)
		{
			write(frm_fd, bitmap, frms[i]->frmlen * 2);
			close(frm_fd);
		}
		delete [] bitmap;
		// make option.value
		len = (frms[i]->frmOffset + 2);
		v = new char[len * 2 + 1];
		Cnvt::ParseToAsc(frms[i]->frmData, v, frms[i]->frmOffset);
		Cnvt::ParseToAsc(frms[i]->frmData+frms[i]->frmOffset, v+frms[i]->frmOffset*2, 2);
		v[len * 2] = '\0';
	}

	char option[8];
	sprintf(option, "frm_%d", i);
    Save(SECTION.c_str(), option, v);	
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
	OpenSectionForSave(SECTION.c_str());
	ptrSecSave.option = option;
	ptrSecSave.value = v;
	for (int i = 1; i <= 255; i++)
	{
		sprintf(option, "frm_%d", i);
		Cnvt::ParseToAsc(frms[i]->frmData, v, len);
		v[len * 2] = '\0';
		SetByPtr();
	}
	CloseSectionForSave();

#endif
}
