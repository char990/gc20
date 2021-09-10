#ifndef __UCILOG_H__
#define __UCILOG_H__

#include <uci/UciCfg.h>

class UciLog : public UciCfg
{
public:
    UciLog();
    virtaul ~UciLog();

	const char * SECTION;

    void LoadConfig() override;

	void Dump() override;

private:

}

#endif
