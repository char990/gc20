#include <cstdio>
#include <cstring>
#include <uci/UciFrm.h>
#include <module/Utils.h>
#include <uci/uci.h>


using namespace Utils;

UciFrm::UciFrm()
{
}

void UciFrm::LoadConfig()
{
	chkSum = 0;
	struct uci_context *ctx = uci_alloc_context();
	uci_set_confdir(ctx, PATH);
	struct uci_package *pkg = NULL;
	if (UCI_OK != uci_load(ctx, PACKAGE, &pkg))
	{
		printf("Fatal: can't load uci user config\n");
		uci_free_context(ctx);
		exit(-1);
	}
	struct uci_section *section = uci_lookup_section(ctx, pkg, SECTION);
	if (section != NULL)
	{
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
			Frame * frm;
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
				chkSum += frm->crc;
				if (frms[i] != nullptr)
				{
					delete frms[i];
				}
				frms[i]=frm;
			}
			else
			{
				delete frm;
			}
		}
	}
	uci_unload(ctx, pkg);
	uci_free_context(ctx);
	Dump();
}

void UciFrm::Dump()
{
	for (int i = 1; i < 255; i++)
	{
		if (frms[i] != nullptr)
		{
			printf("%s\n", frms[i]->ToString().c_str());
		}
	}
}

uint16_t UciFrm::ChkSum()
{
	return chkSum;
}

Frame *UciFrm::GetFrame(int i)
{
	return frms[i];
}

APP::ERROR UciFrm::SetFrame(uint8_t *buf, int len)
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

	if (frm->appErr == APP::ERROR::AppNoError)
	{
		int i = frm->frmId;
		if (frms[i] != nullptr)
		{
			chkSum -= frms[i]->crc;
			delete frms[i];
		}
		frms[i] = frm;
		chkSum += frms[i]->crc;
		SaveFrame(i);
	}
	else
	{
		delete frm;
	}
}

void UciFrm::SaveFrame(int i)
{
	if(i<1 || i>255)return;
	char option[8];
	sprintf(option,"frm_%03d",i);
	int len = (frms[i]->frmlen+9);
	char *v = new char [len*2+1];
	Cnvt::ParseToAsc(frms[i]->frmData, v, len);

	struct uci_context *ctx = uci_alloc_context();
	uci_set_confdir(ctx, PATH);
	struct uci_ptr ptr;
	ptr.package = PACKAGE,
	ptr.section = SECTION,
	ptr.option = option;
	ptr.value = v;
	uci_set(ctx, &ptr);
	uci_commit(ctx, &ptr.p, true);
	uci_unload(ctx, ptr.p);
	uci_free_context(ctx);
	delete v;
}
