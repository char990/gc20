#ifndef __PRODCFG_H__
#define __PRODCFG_H__

#include <uci/UciCfg.h>

class UciProd : public UciCfg
{
public:
    UciProd();
    char const* path = "/etc/config";
    char const* package = "goblin";

    void LoadConfig() override;
	void Dump() override;

    uint8_t TsiSp003Ver();
    char * MfcCode();
};

#endif
