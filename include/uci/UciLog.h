#pragma once


#include <uci/UciCfg.h>

class UciLog : public UciCfg
{
public:
    UciLog(){};
    virtual ~UciLog(){};

    virtual void LoadConfig() override = 0;

	virtual void Dump() override {} ;

protected:
    int lastLog{-1};
};

