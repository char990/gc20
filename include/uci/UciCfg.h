#ifndef __UCICFG_H__
#define __UCICFG_H__

#include <cstdint>
#include <string>
#include <uci.h>
#include <module/Utils.h>

struct OptChars
{
	const char * option;
	const char * chars;
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
	char bufSecSave[256];

	/// config dir, nullptr for default(/etc/config)
	const char * PATH;

	/// package(file) name
	const char * PACKAGE;

	/// section name name
    const char * SECTION;

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
	/// \return	is section or option found
	/// \throw	If can't load path/package
	///	 * Note: uci_lookup_ptr will automatically load a config package if necessary
	bool LoadPtr(const char * section);
	bool LoadPtr(const char * section, const char * option);

	/// \brief	Set section.option.value by ptrSecSave
	/// \throw	uci_set failed
	void SetByPtr();

	/// \brief	Save a section.option.value, open/close inside the function
	/// \param	section : section
	/// \param	option : option
	/// \param	value : value
	/// \throw	If can't load path/package
	void Save(const char * section, const char * option, const char * value);
	void Save(const char * section, struct OptChars * optval);
	void Save(const char * section, struct OptChars ** optval, int len);

	/// \brief	save multi option.value
	///			steps: OpenSectionForSave(), then OptionSave(), ... , then CloseSectionForSave()
	/// \param	option : option
	/// \param	value : value
	/// \throw	If can't load path/package
	void OpenSectionForSave(const char * section);
	/// int will be changed to '123435' then Option_Save()
	void OptionSave(const char * option, int value);
	/// chars will be changed to 'chars' then Option_Save()
	/// Note: Single(') and double(") quotes are not allowed in chars
	void OptionSave(const char * option, const char * chars);
	//void OptionSave(struct OptChars * optval);
	//void OptionSave(struct OptChars ** optval, int len);
	void CloseSectionForSave();

	void PrintOption_2x(const char * option, int x);
	void PrintOption_4x(const char * option, int x);
	void PrintOption_d(const char * option, int x);
	void PrintOption_f(const char * option, float x);
	void PrintOption_str(const char * option, const char * str);

private:
	/// Lower of OptionSave. Do not call this funtion
	void Option_Save(const char * option, const char * str);
};


#endif
