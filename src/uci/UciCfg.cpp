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
	if (ctx != nullptr)
	{
		uci_free_context(ctx);
		ctx = nullptr;
	}
}

/*
int UciCfg::uci_lookup_option_int ( struct uci_context * uci, struct uci_section * s, const char * name,
									   int default_value )
{
	const char * str = uci_lookup_option_string ( uci, s, name );

	return str == NULL ? default_value : strtol ( str, NULL, 0 );
}

unsigned int UciCfg::uci_lookup_option_uint ( struct uci_context * uci, struct uci_section * s, const char * name,
		unsigned int default_value )
{
	const char * str = uci_lookup_option_string ( uci, s, name );

	return str == NULL ? default_value : strtoul ( str, NULL, 0 );
}

float UciCfg::uci_lookup_option_float ( struct uci_context * uci, struct uci_section * s, const char * name,
		float default_value )
{
	const char * str = uci_lookup_option_string ( uci, s, name );

	return str == NULL ? default_value : strtof32 ( str, NULL);
}
*/

#define UCI_BUF_SIZE 1024

int UciCfg::UciGet(char *buf, int buflen)
{
	char cmd[UCI_BUF_SIZE];
	snprintf(cmd, UCI_BUF_SIZE - 1, "uci -c %s get %s.%s.%s",
			 uciOpt.path, uciOpt.package, uciOpt.section, uciOpt.option);
	return Utils::Exec::Run(cmd, buf, buflen);
}

int UciCfg::UciSet()
{
	char buf[UCI_BUF_SIZE];
	snprintf(buf, UCI_BUF_SIZE - 1, "uci -c %s set %s.%s.%s=%s",
			 uciOpt.path, uciOpt.package, uciOpt.section, uciOpt.option, uciOpt.value);
	printf("%s\n", buf);
	return system(buf);
}

int UciCfg::UciCommit()
{
	char buf[UCI_BUF_SIZE];
	snprintf(buf, UCI_BUF_SIZE - 1, "uci -c %s commit %s", uciOpt.path, uciOpt.package);
	return system(buf);
}

void UciCfg::Open(const char *path, const char *package)
{
	ctx = uci_alloc_context();
	if(ctx==nullptr)
	{
		MyThrow("Open '%s/%s' error. Can't alloc context.", path, package);
	}
	if (path != nullptr)
	{
		uci_set_confdir(ctx, path);
	}
	if (UCI_OK != uci_load(ctx, package, &pkg))
	{
		uci_free_context(ctx);
		MyThrow("Can't load '%s/%s'.", path, package);
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

bool UciCfg::GetPtr(struct uci_ptr *ptr, const char * section, char *buf)
{
	snprintf(buf, 255, "%s.%s", pkg, section);
	if (uci_lookup_ptr(ctx, ptr, buf, true) != UCI_OK)
	{
		MyThrow("Can't load package:%s", pkg);
	}
	return (ptr->flags & uci_ptr::UCI_LOOKUP_COMPLETE)!=0;
}

bool UciCfg::GetPtr(struct uci_ptr *ptr, const char * section, const char * option, char *buf)
{
	snprintf(buf, 255, "%s.%s.%s", pkg, section, option);
	if (uci_lookup_ptr(ctx, ptr, buf, true) != UCI_OK)
	{
		MyThrow("Can't load package:%s", pkg);
	}
	return (ptr->flags & uci_ptr::UCI_LOOKUP_COMPLETE)!=0;
}
