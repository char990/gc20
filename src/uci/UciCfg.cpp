#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <uci/UciCfg.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

UciCfg::UciCfg()
	: ctx(nullptr)
{
}

UciCfg::~UciCfg()
{
	Close();
}

struct uci_section *UciCfg::GetSection(const char *name)
{
	if (ctx == nullptr)
	{
		Open();
	}
	struct uci_section *sec = uci_lookup_section(ctx, pkg, name);
	if (sec == NULL)
	{
		MyThrow("Can't load %s/%s.%s", PATH.c_str(), PACKAGE.c_str(), name);
	}
	return sec;
}

struct uci_section *UciCfg::GetSection(const char *type, const char *name)
{
	struct uci_section *sec;
	while(1)
	{
		sec = GetSection(name);
		if (strcmp(type, sec->type) == 0)
		{
			return sec;
		}
	}
}

const char *UciCfg::GetStr(struct uci_section *section, const char *option)
{
	if (ctx == nullptr || section == nullptr)
	{
		MyThrow("OPEN '%s/%s' and get *section before calling", PATH.c_str(), PACKAGE.c_str());
	}
	return uci_lookup_option_string(ctx, section, option);
}

int UciCfg::GetInt(struct uci_section *section, const char *option, int min, int max)
{
	const char *str = GetStr(section, option);
	if(str==NULL)
	{
		MyThrow("Can't find option: %s.%s.%s", PACKAGE.c_str(), section, option);
	}
	errno=0;
	int x = strtol(str, NULL, 0);
	if (errno!=0 || x < min || x > max)
	{
		MyThrow("Invalid option: %s.%s.%s='%d' [%d-%d]", PACKAGE.c_str(), section, option, x, min, max);
	}
	return x;
}

uint32_t UciCfg::GetUint32(struct uci_section *section, const char *option, uint32_t min, uint32_t max)
{
	const char *str = GetStr(section, option);
	if(str==NULL)
	{
		MyThrow("Can't find option: %s.%s.%s", PACKAGE.c_str(), section, option);
	}
	errno=0;
	uint32_t x = strtoul(str, NULL, 0);
	if (errno!=0 || x < min || x > max)
	{
		MyThrow("Invalid option: %s.%s.%s='%u' [%u-%u]", PACKAGE.c_str(), section, option, x, min, max);
	}
	return x;
}

void UciCfg::Open()
{
	ctx = uci_alloc_context();
	if (ctx == nullptr)
	{
		MyThrow("Open '%s/%s' error. Can't alloc context.", PATH.c_str(), PACKAGE.c_str());
	}
	uci_set_confdir(ctx, PATH.c_str());
	if (UCI_OK != uci_load(ctx, PACKAGE.c_str(), &pkg))
	{
		uci_free_context(ctx);
		ctx = nullptr;
		pkg = nullptr;
		MyThrow("Can't load '%s/%s'.", PATH.c_str(), PACKAGE.c_str());
	}
}

void UciCfg::Commit()
{
	if (ctx != nullptr && pkg != nullptr)
	{
		uci_commit(ctx, &pkg, false);
	}
}

void UciCfg::Close()
{
	if (ctx != nullptr)
	{
		if (pkg != nullptr)
		{
			uci_unload(ctx, pkg);
			pkg = nullptr;
		}
		uci_free_context(ctx);
		ctx = nullptr;
	}
}

bool UciCfg::LoadPtr(const char *section, char *buf)
{
	snprintf(buf, 255, "%s.%s", PACKAGE.c_str(), section);
	if (uci_lookup_ptr(ctx, &ptrSecSave, buf, true) != UCI_OK)
	{
		MyThrow("Can't load package:%s", PACKAGE.c_str());
	}
	return (ptrSecSave.flags & uci_ptr::UCI_LOOKUP_COMPLETE) != 0;
}

bool UciCfg::LoadPtr(const char *section, const char *option, char *buf)
{
	snprintf(buf, 255, "%s.%s.%s", PACKAGE.c_str(), section, option);
	if (uci_lookup_ptr(ctx, &ptrSecSave, buf, true) != UCI_OK)
	{
		MyThrow("Can't load package:%s", PACKAGE.c_str());
	}
	return (ptrSecSave.flags & uci_ptr::UCI_LOOKUP_COMPLETE) != 0;
}

void UciCfg::Save(const char *section, const char *option, const char *value)
{
	OpenSectionForSave(section);
	ptrSecSave.option = option;
	ptrSecSave.value = value;
	SetByPtr();
	CloseSectionForSave();
}

void UciCfg::Save(const char *section, struct OptVal *optval)
{
	Save(section, optval->option, optval->value);
}

void UciCfg::Save(const char *section, struct OptVal **optval, int len)
{
	OpenSectionForSave(section);
	for (int i = 0; i < len; i++)
	{
		ptrSecSave.option = optval[i]->option;
		ptrSecSave.value = optval[i]->value;
		SetByPtr();
	}
	CloseSectionForSave();
}

void UciCfg::SetByPtr()
{
	int r = uci_set(ctx, &ptrSecSave);
	if(r!=UCI_OK)
	{
		MyThrow ("SetByPtr failed(return %d): %s.%s.%s=%s", r,
			ptrSecSave.package, ptrSecSave.section, ptrSecSave.option, ptrSecSave.value);
	}
}

void UciCfg::OpenSectionForSave(const char *section)
{
	Open();
	bufSecSave = new char[256];
	if (!LoadPtr(section, bufSecSave))
	{
		MyThrow("OpenSectionForSave failed : section=%s", section);
	}
}

void UciCfg::CloseSectionForSave()
{
	Commit();
	Close();
	if (bufSecSave != nullptr)
	{
		delete[] bufSecSave;
		bufSecSave = nullptr;
	}
}

void UciCfg::OptionSave(const char *option, const char *value)
{
	ptrSecSave.option = option;
	ptrSecSave.value = value;
	SetByPtr();
}

void UciCfg::OptionSave(struct OptVal *optval)
{
	OptionSave( optval->option, optval->value);
}

void UciCfg::OptionSave(struct OptVal **optval, int len)
{
	for (int i = 0; i < len; i++)
	{
		OptionSave(optval[i]);
	}
}

void UciCfg::OptionSaveInt(const char * option, int value) 
{
	char buf[32];
	sprintf(buf,"'%d'",value);
	OptionSave(option, buf);
}

void UciCfg::OptionSaveWord(const char * option, char * word) 
{
	char buf[32];
	sprintf(buf,"'%s'",word);
	OptionSave(option, buf);
}

void UciCfg::PrintOption_2x(const char * option, int x)
{
	printf("\t%s '0x%02X'\n", option, x);
}

void UciCfg::PrintOption_4x(const char * option, int x)
{
	printf("\t%s '0x%04X'\n", option, x);
}

void UciCfg::PrintOption_d(const char * option, int x)
{
	printf("\t%s '%d'\n", option, x);
}
