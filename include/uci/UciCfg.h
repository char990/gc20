#ifndef __UCICFG_H__
#define __UCICFG_H__

#include <cstdint>

struct UciOpt
{
	char * path;
	char * package;
	char * section;
	char * option;
	char * value;
};

class UciCfg
{
public:
	UciCfg();
	virtual ~UciCfg();
	virtual void LoadConfig() = 0;
	virtual void Dump() = 0;

protected:
/*
	int uci_lookup_option_int ( struct uci_context * uci, struct uci_section * s, const char * name,
								int default_value = 0 );
	unsigned int uci_lookup_option_uint ( struct uci_context * uci, struct uci_section * s, const char * name,
										  unsigned int default_value = 0 );
	float uci_lookup_option_float ( struct uci_context * uci, struct uci_section * s, const char * name,
										  float default_value = 0 );

	struct uci_context * ctx;
*/
	/// \brief	Get option
	/// \return >0:sucess, size of buf; -1:failed
	int UciGet(char * buf, int buflen);

	/// \brief	Set option
	/// \return 0:sucess; others:failed:return of uci
	int UciSet();

	/// \brief	Commit all settings/changes
	/// \return 0:sucess; others:failed:return of uci
	int UciCommit();
	
	/// \brief	
	struct UciOpt uciOpt;
};

#endif
