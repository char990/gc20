#ifndef __UCICFG_H__
#define __UCICFG_H__

#include <cstdint>
#include <string>
#include <uci.h>
#include <module/Utils.h>

struct OptVal
{
	const char * option;
	const char * value;
};
class UciCfg
{
public:
	UciCfg();
	virtual ~UciCfg();
	virtual void LoadConfig() = 0;
	virtual void Dump() = 0;

	/// Get section, must call Open() before using
    struct uci_section * GetSection(const char *name);						// don't care section type
    struct uci_section * GetSection(const char *type, const char *name);	// both type & name should match

	/// Get option in section, must call Open() before using
	const char * GetStr(struct uci_section * section, const char * option);
	int GetInt(struct uci_section * section, const char * option, int min, int max);
	uint32_t GetUint32(struct uci_section * section, const char * option, uint32_t min, uint32_t max);
    void ReadBitOption(struct uci_section *section, const char * option, Utils::BitOption &bo);
    int GetIntFromStrz(struct uci_section * uciSec, const char *option, const char **collection, int cSize);

protected:
	
	struct uci_context *ctx;
	struct uci_package *pkg;
	struct uci_ptr ptrSecSave;
	char * bufSecSave;

	/// config dir, nullptr for default(/etc/config)
	const char * PATH;
	/// package(file) name
	const char * PACKAGE;

	/// \brief	Open a package, context=>ctx, package=>pkg
	/// \throw	If can't load path/package
	void Open();
	
	/// \brief	Commit all changes
	void Commit();

	/// \brief	Close
	void Close();

	/// \brief	get ptr for uci_set
	/// \param	section : section
	/// \param	option : option
	/// \param	buf : char buf[256] to store element
	/// \return	is section or option found
	/// \throw	If can't load path/package
	///	 * Note: uci_lookup_ptr will automatically load a config package if necessary
	///	 * @buf must not be constant, as it will be modified and used for the strings inside @ptr,
	///	 * thus it must also be available as long as @ptr is in use.
	bool LoadPtr(const char * section, char *buf);
	bool LoadPtr(const char * section, const char * option, char *buf);

	/// \brief	Set section.option.value by ptrSecSave
	/// \throw	uci_set failed
	void SetByPtr();

	/// \brief	Save a section.option.value, open/close inside the function
	/// \param	section : section
	/// \param	option : option
	/// \param	value : value
	/// \throw	If can't load path/package
	void Save(const char * section, const char * option, const char * value);
	void Save(const char * section, struct OptVal * optval);
	void Save(const char * section, struct OptVal ** optval, int len);

	/// \brief	save multi option.value
	///			steps: OpenSectionForSave(), then OptionSave(), ... , then CloseSectionForSave()
	/// \param	option : option
	/// \param	value : value
	/// \throw	If can't load path/package
	void OpenSectionForSave(const char * section);
	void OptionSave(const char * option, const char * value);
	void OptionSave(struct OptVal * optval);
	void OptionSave(struct OptVal ** optval, int len);
	void OptionSaveInt(const char * option, int value);
	/// chars will be changed to 'chars' then OptionSave()
	void OptionSaveChars(const char * option, const char * chars);
	void CloseSectionForSave();

	void PrintOption_2x(const char * option, int x);
	void PrintOption_4x(const char * option, int x);
	void PrintOption_d(const char * option, int x);
	void PrintOption_f(const char * option, float x);
	void PrintOption_str(const char * option, const char * str);
};


#endif
