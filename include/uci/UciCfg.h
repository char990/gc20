#ifndef __UCICFG_H__
#define __UCICFG_H__

#include <cstdint>
#include <string>

struct OptVal
{
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

	/// Get section, must call Open() before using
    struct uci_section * GetSection(const char *name);						// don't care section type
    struct uci_section * GetSection(const char *type, const char *name);	// both type & name should match

	/// Get option in section, must call Open() before using
	const char * GetStr(struct uci_section * section, const char * option);
	int GetInt(struct uci_section * section, const char * option, int default, int min, int max);
	uint32_t GetUint32(struct uci_section * section, const char * option, uint32_t default,uint32_t min, uint32_t max);
	float GetFloat(struct uci_section * section, const char * option, float default);

protected:
	
	struct uci_context *ctx;
	struct uci_package *pkg;

	/// config dir, nullptr for default(/etc/config)
	std::string PATH;
	/// package(file) name
	std::string PACKAGE;

	/// \brief	Open a package, context=>ctx, package=>pkg
	/// \throw	If can't load path/package
	void Open();
	
	/// \brief	Commit all changes
	void Commit();

	/// \brief	Close
	void Close();

	/// \brief	get ptr for uci_set
	/// \param	ptr : uci_ptr
	/// \param	section : section
	/// \param	option : option
	/// \param	buf : char buf[256] to store element
	/// \return	is section or option found
	/// \throw	If can't load path/package
	///	 * Note: uci_lookup_ptr will automatically load a config package if necessary
	///	 * @buf must not be constant, as it will be modified and used for the strings inside @ptr,
	///	 * thus it must also be available as long as @ptr is in use.
	bool GetPtr(struct uci_ptr *ptr, const char * section, char *buf);
	bool GetPtr(struct uci_ptr *ptr, const char * section, const char * option, char *buf);

	/// \brief	Save section.option.value
	/// \param	section : section
	/// \param	option : option
	/// \param	value : value
	/// \throw	If can't load path/package
	void Save(const char * section, char * option, char * value);
	void Save(const char * section, struct OptVal * optval);
	void Save(const char * section, struct OptVal ** optval, int len);

	/// \brief	OpenSectionForSave(), then OptionSave(), ... , then CloseSectionForSave()
	/// \param	option : option
	/// \param	value : value
	/// \throw	If can't load path/package
	char * bufSecSave;
	void OpenSectionForSave(const char * section);
	void OptionSave(char * option, char * value);
	void OptionSave(struct OptVal * optval);
	void OptionSave(struct OptVal ** optval, int len);
	void OptionSaveInt(char * option, int value);
	void CloseSectionForSave();
};


#endif
