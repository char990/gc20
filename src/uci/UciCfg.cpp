#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <uci/uci.h>
#include <uci/UciCfg.h>
#include <module/Utils.h>



UciCfg::UciCfg()
//	: ctx ( NULL )
{
//	ctx = uci_alloc_context();
}

UciCfg::~UciCfg()
{
	//uci_free_context ( ctx );
	//ctx = NULL;
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

int UciCfg::UciGet(char * buf, int buflen)
{
	char cmd[UCI_BUF_SIZE];
    snprintf(cmd,UCI_BUF_SIZE-1,"uci -c %s get %s.%s.%s",
		uciOpt.path, uciOpt.package, uciOpt.section, uciOpt.option);
	return Utils::Exec::Run(cmd, buf, buflen);
}

int UciCfg::UciSet()
{
	char buf[UCI_BUF_SIZE];
	snprintf(buf,UCI_BUF_SIZE-1,"uci -c %s set %s.%s.%s=%s",
		uciOpt.path, uciOpt.package, uciOpt.section, uciOpt.option, uciOpt.value);
	printf("%s\n",buf);
	return system(buf);
}

int UciCfg::UciCommit()
{
	char buf[UCI_BUF_SIZE];
	snprintf(buf,UCI_BUF_SIZE-1,"uci -c %s commit %s", uciOpt.path, uciOpt.package);
	return system(buf);
}
