#ifndef __PRODCFG_H__
#define __PRODCFG_H__

#include <uci/UciCfg.h>

class UciProd : public UciCfg
{
public:
    UciProd();
    char const* PATH = "/etc/config";
    char const* PACKAGE = "goblin";
    char const* SECTION = "frm";

    void LoadConfig() override;
	void Dump() override;

    uint8_t TsiSp003Ver();
    char * MfcCode();

    int ColourBits();
    
    int MaxTextFrmLen();
    int MinGfxFrmLen();
    int MaxGfxFrmLen();
    int MinHrgFrmLen();
    int MaxHrgFrmLen();

    int CharRows();
    int CharColumns();

    int PixelRows();
    int PixelColumns();
    int Pixels();
};

#endif
