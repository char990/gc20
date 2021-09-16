#ifndef __UCILOG_H__
#define __UCILOG_H__

#include <uci/UciCfg.h>

class UciLog : public UciCfg
{
public:
    UciLog():lastLog(-1){};
    virtual ~UciLog(){};

    const char * _LastLog = "LastLog";
    virtual void LoadConfig() override = 0;

	virtual void Dump() override {} ;

    virtual void SaveLastLog(char * chars)
    {
        OpenSectionForSave(SECTION);
        char option[16];
        sprintf(option, "%s_%d", SECTION, lastLog);
        OptionSave(option, chars);
        sprintf(option, "%d", lastLog);
        OptionSave(_LastLog, option);
        CloseSectionForSave();
    };

protected:
    int lastLog;
};

#endif
