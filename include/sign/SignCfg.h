#ifndef __SIGNCFG_H__
#define __SIGNCFG_H__

#include <cstdint>

#define SIGNTYPE_SIZE 2
extern const char *SIGNTYPE[SIGNTYPE_SIZE];

/// \brief  Sign's hardware configurations
class SignCfg
{
public:
    SignCfg(uint8_t signType);
    uint8_t SignType() { return signType; };
    
    void MakeSignType0();
    void MakeSignType1();

    int MaxTextFrmLen();
    int MinGfxFrmLen();
    int MaxGfxFrmLen();
    int MinHrgFrmLen();
    int MaxHrgFrmLen();

    int CharRows() { return charRows; };
    int CharColumns() { return charColumns; };

    int PixelRows() { return pixelRows; };
    int PixelColumns() { return pixelColumns; };
    int Pixels() { return pixelRows*pixelColumns};

private:
    uint8_t signType;
   
    int pixelRowsPerTile;
    int pixelColumnsPerTile;
    int tileRowsPerSlave;
    int tileColumnsPerSlave;
    int slaveRowsPerScreen;
    int slaveColumnsPerScreen;

    int pixelRows;              // = pixelRowsPerTile * tileRowsPerSlave * slaveRowsPerScreen
    int pixelColumns;    // = pixelColumnsPerTile * tileColumnsPerSlave * slaveColumnsPerScreen

    int rowsChar;               // for default font with inter-char spacing
    int columnsChar;            // for default font with inter-line spacing

    
};

#endif
