#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <uci.h>
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
	struct uci_section *sec = uci_lookup_section(ctx, pkg, name);
	if (sec == NULL)
	{
		MyThrow("Can't load %s/%s.%s", PATH.c_str(), PACKAGE.c_str(), section);
	}
	return sec;
}

struct uci_section *UciCfg::GetSection(const char *type, const char *name)
{
	struct uci_section *sec = uci_lookup_section(ctx, pkg, name);
	if (sec == NULL || strcmp(type, sec->type) != 0)
	{
		MyThrow("Can't load %s/%s.%s", PATH.c_str(), PACKAGE.c_str(), section);
	}
	return sec;
}

const char *UciCfg::GetStr(struct uci_section *section, const char *option)
{
	if (ctx == nullptr || section == nullptr)
	{
		MyThrow("OPEN '%s/%s' and get *section before calling", PATH.c_str(), PACKAGE.c_str());
	}
	return uci_lookup_option_string(ctx, section, option);
}

int UciCfg::GetInt(struct uci_section *section, const char *option, int def, int min, int max)
{
	const char *str = GetStr(section, option);
	int x = (str == NULL) ? def : strtol(str, NULL, 0);
	if (x < min || x > max)
	{
		MyThrow("Invalid '%s' = %d [%d-%d]", option, x, min, max);
	}
	return x;
}

uint32_t UciCfg::GetUint32(struct uci_section *section, const char *option, uint32_t def, uint32_t min, uint32_t max)
{
	const char *str = GetStr(section, option);
	uint32_t x = (str == NULL) ? def : strtoul(str, NULL, 0);
	if (x < min || x > max)
	{
		MyThrow("Invalid '%s' = %u [%u-%u]", option, x, min, max);
	}
	return x;
}

float UciCfg::GetFloat(struct uci_section *section, const char *option, float def)
{
	const char *str = GetStr(section, option);
	return str == NULL ? def : strtof(str, NULL);
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

bool UciCfg::GetPtr(struct uci_ptr *ptr, const char *section, char *buf)
{
	snprintf(buf, 255, "%s.%s", pkg, section);
	if (uci_lookup_ptr(ctx, ptr, buf, true) != UCI_OK)
	{
		MyThrow("Can't load package:%s", pkg);
	}
	return (ptr->flags & uci_ptr::UCI_LOOKUP_COMPLETE) != 0;
}

bool UciCfg::GetPtr(struct uci_ptr *ptr, const char *section, const char *option, char *buf)
{
	snprintf(buf, 255, "%s.%s.%s", pkg, section, option);
	if (uci_lookup_ptr(ctx, ptr, buf, true) != UCI_OK)
	{
		MyThrow("Can't load package:%s", pkg);
	}
	return (ptr->flags & uci_ptr::UCI_LOOKUP_COMPLETE) != 0;
}

void UciCfg::Save(const char *section, char *option, char *value)
{
	OpenSectionForSave(section);
	ptr.option = option;
	ptr.value = value;
	uci_set(ctx, &ptr);
	CloseSectionForSave();
	Close();
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
		ptr.option = optval[i]->option;
		ptr.value = optval[i]->value;
		uci_set(ctx, &ptr);
	}
	CloseSectionForSave();
	Close();
}

void UciCfg::OpenSectionForSave(const char *section)
{
	Open();
	bufSecSave = new char[256];
	struct uci_ptr ptr;
	if (!GetPtr(&ptr, section, bufSecSave))
	{
		MyThrow("OpenSectionForSave failed : section=%s", section);
	}
}

void UciCfg::CloseSectionForSave()
{
	Commit();
	if (bufSecSave != nullptr)
	{
		delete[] bufSecSave;
		bufSecSave = nullptr;
	}
}

void UciCfg::OptionSave(char *option, char *value)
{
	ptr.option = option;
	ptr.value = value;
	uci_set(ctx, &ptr);
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

void UciCfg::OptionSaveInt(char * option, int value) 
{
	char buf[32];
	sprintf(buf,"%d",value);
	OptionSave(option, buf);
}

