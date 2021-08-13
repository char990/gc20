#ifndef __SIGNCFG_H__
#define __SIGNCFG_H__

#include <cstdint>

/// \brief  Sign's hardware configurations
class SignCfg
{
public:
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

    /// \brief  colour bits, only 1, 4, 24 allowed
    int colourbit;

    /// \brief  each bit indicates a colour code
    ///         E.g. in "Sign Set High Resolution Graphics Frame"
    ///         0x63FF=0b'0110'0011'1111'1111 => 0:default, 1-9:mono/1-bit, 0x0D:multi/4-bit, 0x0E:24-bit
    uint16_t colours;
    
    /// \brief  for colour code 0-9, [n] is origin colour code, colourMap[n] is mapped code
    ///         E.g. default is Red, Yellow mapped to Amber:   colourMap[0]:1, colourMap[2]:9 
    uint8_t colourMap[10];

    /// \brief  max Conspicuity: 0-5
    uint8_t maxConspicuity;

    /// \brief  max Annulus: 0-2
    uint8_t maxAnnulus;
    
    /// \brief  max Font: 0-5
    uint8_t maxFont;
    
};

#endif
