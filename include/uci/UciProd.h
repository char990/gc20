#ifndef __PRODCFG_H__
#define __PRODCFG_H__

#include <uci/UciCfg.h>

class UciProd : public UciCfg
{
public:
    UciProd();

    bool IsChanged();
    
    void LoadConfig() override;
    void Dump() override;

    uint8_t TsiSp003Ver();
    char *MfcCode();

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

private:
    bool isChanged;
};

#endif
