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


	struct uci_context *ctx;
	struct uci_package *pkg;

	/// \brief	Open a package, context=>ctx, package=>pkg
	/// \param	path : config dir, nullptr for default(/etc/config)
	/// \param	package : package(file) name
	/// \throw	If can't load path/package
	void Open(const char *path, const char *package);
	
	/// \brief	Commit all changes
	void Commit();

	/// \brief	Close
	void Close();

	/// \brief	set ptr for uci_set
	/// \param	ptr : uci_ptr
	/// \param	section : section
	/// \param	option : option
	/// \param	buf : char buf[256] to store element
	/// \throw	If can't load path/package
	void SetPtr(struct uci_ptr *ptr, const char * section, char *buf);
	void SetPtr(struct uci_ptr *ptr, const char * section, const char * option, char *buf);
};

#endif
