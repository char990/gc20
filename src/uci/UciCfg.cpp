#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <uci/UciCfg.h>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>
#include <module/Utils.h>

using namespace Utils;
using namespace std;

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
	struct uci_section *uciSec = uci_lookup_section(ctx, pkg, name);
	if (uciSec == NULL)
	{
		throw invalid_argument(StrFn::PrintfStr("Can't load %s/%s.%s", PATH, PACKAGE, name));
	}
	return uciSec;
}

struct uci_section *UciCfg::GetSection(const char *type, const char *name)
{
	struct uci_section *uciSec;
	uciSec = GetSection(name);
	if (strcmp(type, uciSec->type) != 0)
	{
		throw invalid_argument(StrFn::PrintfStr("Section type not matched: %s/%s.%s : %s!=%s",
												PATH, PACKAGE, name, type, uciSec->type));
	}
	return uciSec;
}

const char *UciCfg::GetStr(struct uci_section *section, const char *option, bool ex)
{
	if (ctx == nullptr || section == nullptr)
	{
		throw invalid_argument(StrFn::PrintfStr("OPEN '%s/%s' and get *section before calling", PATH, PACKAGE));
	}
	const char *str = uci_lookup_option_string(ctx, section, option);
	if (str == NULL && ex)
	{
		throw invalid_argument(StrFn::PrintfStr("Uci Error: %s/%s.%s.%s is not defined", PATH, PACKAGE, section->e.name, option));
	}
	return str;
}

int UciCfg::GetInt(struct uci_section *section, const char *option, int min, int max, bool ex)
{
	const char *str = GetStr(section, option, ex);
	if (str == NULL)
	{
		if (ex)
		{
			throw invalid_argument(StrFn::PrintfStr("Uci Error: %s/%s.%s.%s is not defined",
													PATH, PACKAGE, section->e.name, option));
		}
		else
		{
			return 0;
		}
	}
	errno = 0;
	int x = strtol(str, NULL, 0);
	if (errno != 0 || x < min || x > max)
	{
		throw invalid_argument(StrFn::PrintfStr("Invalid option: %s.%s.%s='%d' [%d-%d]",
												PACKAGE, section->e.name, option, x, min, max));
	}
	return x;
}

void UciCfg::Open()
{
	ctx = uci_alloc_context();
	if (ctx == nullptr)
	{
		throw invalid_argument(StrFn::PrintfStr("Open '%s/%s' error. Can't alloc context.", PATH, PACKAGE));
	}
	uci_set_confdir(ctx, PATH);
	uci_set_savedir(ctx, _SAVEDIR);
	if (UCI_OK != uci_load(ctx, PACKAGE, &pkg))
	{
		uci_free_context(ctx);
		ctx = nullptr;
		pkg = nullptr;
		throw invalid_argument(StrFn::PrintfStr("Can't load '%s/%s'.", PATH, PACKAGE));
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

bool UciCfg::LoadPtr(const char *section)
{
	snprintf(bufSecSave, PRINT_BUF_SIZE - 1, "%s.%s", PACKAGE, section);
	if (uci_lookup_ptr(ctx, &ptrSecSave, bufSecSave, true) != UCI_OK)
	{
		throw invalid_argument(StrFn::PrintfStr("Can't load package:%s", PACKAGE));
	}
	return (ptrSecSave.flags & uci_ptr::UCI_LOOKUP_COMPLETE) != 0;
}

bool UciCfg::LoadPtr(const char *section, const char *option)
{
	snprintf(bufSecSave, PRINT_BUF_SIZE - 1, "%s.%s.%s", PACKAGE, section, option);
	if (uci_lookup_ptr(ctx, &ptrSecSave, bufSecSave, true) != UCI_OK)
	{
		throw invalid_argument(StrFn::PrintfStr("Can't load package:%s", PACKAGE));
	}
	return (ptrSecSave.flags & uci_ptr::UCI_LOOKUP_COMPLETE) != 0;
}

void UciCfg::OpenSaveClose(const char *section, const char *option, int value)
{
	OpenSectionForSave(section);
	OptionSave(option, value);
	CommitCloseSectionForSave();
}

void UciCfg::OpenSaveClose(const char *section, const char *option, const char *value)
{
	OpenSectionForSave(section);
	OptionSave(option, value);
	CommitCloseSectionForSave();
}

void UciCfg::OpenSaveClose(const char *section, struct OptChars *optval)
{
	OpenSaveClose(section, optval->option, optval->chars);
}

/*
void UciCfg::OpenSaveClose(const char *section, struct OptChars **optval, int len)
{
	OpenSectionForSave(section);
	for (int i = 0; i < len; i++)
	{
		ptrSecSave.option = optval[i]->option;
		ptrSecSave.value = optval[i]->chars;
		SetByPtr();
		OptionSave(optval[i]->option, optval[i]->chars);
	}
	CommitCloseSectionForSave();
}
*/

void UciCfg::OpenSaveClose(const char *section, const char *option, Utils::Bits &bo)
{
	OpenSectionForSave(section);
	OptionSave(option, bo.ToString().c_str());
	CommitCloseSectionForSave();
}

void UciCfg::SetByPtr()
{
	int r = uci_set(ctx, &ptrSecSave);
	if (r != UCI_OK)
	{
		throw runtime_error(StrFn::PrintfStr("SetByPtr failed(return %d): %s.%s.%s=%s", r,
											 ptrSecSave.package, ptrSecSave.section, ptrSecSave.option, ptrSecSave.value));
	}
}

void UciCfg::OpenSectionForSave(const char *section)
{
	Open();
	if (!LoadPtr(section))
	{
		throw invalid_argument(StrFn::PrintfStr("OpenSectionForSave failed : section=%s", section));
	}
}

void UciCfg::CommitCloseSectionForSave()
{
	Commit();
	Close();
	// Changed(true);
	// DbHelper::Instance().RefreshSync();
}

void UciCfg::OptionSave(const char *option, const char *str)
{
	ptrSecSave.option = option;
	ptrSecSave.value = str;
	SetByPtr();
}

void UciCfg::OptionSave(const char *option, int value)
{
	OptionSave(option, std::to_string(value).c_str());
}

/*
void UciCfg::OptionSave(struct OptChars *optchars)
{
	OptionSave( optchars->option, optchars->chars);
}

void UciCfg::OptionSave(struct OptChars **optchars, int len)
{
	for (int i = 0; i < len; i++)
	{
		OptionSave(optchars[i]);
	}
}
*/
void UciCfg::PrintOption_2x(const char *option, int x)
{
	printf("\t%s \t'0x%02X'\n", option, x);
}

void UciCfg::PrintOption_4x(const char *option, int x)
{
	printf("\t%s \t'0x%04X'\n", option, x);
}

void UciCfg::PrintOption_d(const char *option, int x)
{
	printf("\t%s \t'%d'\n", option, x);
}

void UciCfg::PrintOption_f(const char *option, float x)
{
	printf("\t%s \t'%f'\n", option, x);
}

void UciCfg::PrintOption_str(const char *option, const char *str)
{
	printf("\t%s \t'%s'\n", option, str);
}

void UciCfg::ReadBits(struct uci_section *uciSec, const char *option, Utils::Bits &bo, bool ex)
{
	int ibuf[256];
	if (bo.Size() == 0 || bo.Size() > 256)
	{
		throw invalid_argument(StrFn::PrintfStr("Uci Error: %s.%s, ReadBits: bo.size=%d", uciSec->e.name, option, bo.Size()));
	}
	bo.ClrAll();
	try
	{
		const char *str = GetStr(uciSec, option);
		if (str != NULL)
		{
			int cnt = Utils::Cnvt::GetIntArray(str, bo.Size(), ibuf, 0, bo.Size() - 1);
			if (cnt != 0)
			{
				for (int i = 0; i < cnt; i++)
				{
					bo.SetBit(ibuf[i]);
				}
				return;
			}
		}
		throw invalid_argument(StrFn::PrintfStr("Uci Error: %s.%s", uciSec->e.name, option));
	}
	catch (exception e)
	{
		if (ex)
		{
			throw e;
		}
	}
}

int UciCfg::GetIndexFromStrz(struct uci_section *uciSec, const char *option, const char **collection, int cSize)
{
	const char *str = GetStr(uciSec, option);
	int x = Utils::Pick::PickStr(str, collection, cSize);
	if (x == -1)
		throw invalid_argument(StrFn::PrintfStr("Uci Error: option %s.%s '%s' is not a valid value", uciSec->e.name, option, str));
	return x;
}

int UciCfg::GetInt(struct uci_section *uciSec, const char *option, const int *collection, int cSize, bool ex)
{
	int x = GetInt(uciSec, option, INT_MIN, INT_MAX, ex);
	if (std::find (collection, collection+cSize, x) != collection+cSize)
	{
		return x;
	}
	else if (ex)
	{
		throw invalid_argument(StrFn::PrintfStr("Uci Error: option %s.%s '%d' is not a valid value", uciSec->e.name, option, x));
	}
	return 0;
}

void UciCfg::ClrSECTION()
{
	char dst[PRINT_BUF_SIZE];
	snprintf(dst, PRINT_BUF_SIZE - 1, "%s/%s", PATH, PACKAGE);
	int dstfd = open(dst, O_WRONLY | O_TRUNC, 0660);
	if (dstfd < 0)
	{
		throw runtime_error(StrFn::PrintfStr("Can't open %s to write", dst));
	}
	int len = snprintf(dst, PRINT_BUF_SIZE - 1, "config %s '%s'\n", PACKAGE, SECTION);
	if (len > PRINT_BUF_SIZE - 1)
		len = PRINT_BUF_SIZE - 1;
	write(dstfd, &dst[0], len);
	fdatasync(dstfd);
	close(dstfd);
}

void UciCfg::ThrowError(const char *option, const char *str)
{
	throw invalid_argument(StrFn::PrintfStr("%s.%s.%s error:%s", PACKAGE, SECTION, option, str));
}

void UciCfg::ThrowError(const char *option, const int i)
{
	throw invalid_argument(StrFn::PrintfStr("%s.%s.%s error:%d", PACKAGE, SECTION, option, i));
}
