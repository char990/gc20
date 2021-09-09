#ifndef __FONT_H__
#define __FONT_H__

#include <cstdint>

#define MAX_FONT_DOT (64*64)

/********* Font definition *********
1. Font file name looks like P5X7/F12X20
2. File name length is 1-15
3. Font file is binary file
4. First 8-byte is font info
    bytesPerCell=buf[0]*0x100+buf[1];
    columnsPerCell=buf[2];
    rowsPerCell=buf[3];
    bytesPerCellRow=buf[4];
    charSpacing=buf[5];
    lineSpacing=buf[6];
    descender=buf[7];
5. From 9th byte, cell(char) data starts
6. Each cell is bytesPerCell's long
7. In each cell, celldata[0]&0x80 is descender flag for this char
8. In each cell, celldata[0]&0x7F is columns of the dot matrix
9. In each cell, celldata[1-n] are dot matrix
***********************************/

    /* cell data is a uint8_t array, length = bytesPerCell
    [0]: bit7: with decender, bit0-6: dot columns

    [1 - (bytesPerCell-1)] : dot data
    data is arranged such that the leftmost column of the character is in the most significant bit of the 2-byte required.

    E.g. character '/' in a 10 columns x 11 rows font
    row uses 16 binary bits (2-byte)

      1234567890123456 ( x represents an unused bit)
    1 .......●●.xxxxxx
    2 .......●●.xxxxxx
    3 ......●●●.xxxxxx
    4 .....●●●..xxxxxx
    5 ....●●●...xxxxxx
    6 ...●●●....xxxxxx
    7 ..●●●.....xxxxxx
    8 .●●●......xxxxxx
    9 ●●●.......xxxxxx
    0 ●●........xxxxxx
    1 ●●........xxxxxx

    converting to a more compact hex form produces 10 rows of 2 bytes each ;
    0x01,0x80,
    0x01,0x80,
    0x03,0x80,
    0x07,0x00,
    0x0E,0x00,
    0x1C,0x00,
    0x38,0x00,
    0x70,0x00,
    0xE0,0x00,
    0xC0,0x00,
    0xC0,0x00,

    The whole cell data would be :
    {10,0x01,0x80,0x01,0x80,0x03,0x80,0x07,0x00,0x0E,0x00,0x1C,0x00,0x38,0x00,0x70,0x00,0xE0,0x00,0xC0,0x00,0xC0,0x00}


    E.g. character 'g' in a 12 columns x 16 rows font
    row uses 16 binary bits (2-byte)

    1234567890123456 ( x represents an unused bit)
    1 ............xxxx
    2 ............xxxx
    3 ...●●●●●....xxxx
    4 ..●●●●●●●...xxxx
    5 ..●●...●●...xxxx
    6 ..●●...●●...xxxx
    7 ..●●...●●...xxxx
    8 ..●●...●●...xxxx
    9 ..●●...●●...xxxx
    0 ..●●...●●...xxxx
    1 ..●●●●●●●...xxxx
    2 ...●●●●●●...xxxx
    3 .......●●...xxxx
    4 .......●●...xxxx
    5 ...●●●●●●...xxxx
    6 ...●●●●●....xxxx

    If there is no decender(=0), the whole cell data would be :
    {12, 0x00,0x00, 0x00,0x00, 0x1F,0x00, 0x3F,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x3F,0x80, 0x1F,0x80, 0x01,0x80, 0x01,0x80, 0x1F,0x80, 0x1F,0x00}

    If there is decender(=4), he whole cell data would be : (just with 0x80 attached to first byte)
    {0x80+12, 0x00,0x00, 0x00,0x00, 0x1F,0x00, 0x3F,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x3F,0x80, 0x1F,0x80, 0x01,0x80, 0x01,0x80, 0x1F,0x80, 0x1F,0x00}
    And dot matrix looks like 12X20 with descender=4 （Insert 4 empty line at head）
    1234567890123456 ( x represents an unused bit)
    1 ............xxxx
    2 ............xxxx
    3 ............xxxx
    4 ............xxxx
    5 ............xxxx
    6 ............xxxx
    7 ...●●●●●....xxxx
    8 ..●●●●●●●...xxxx
    9 ..●●...●●...xxxx
    0 ..●●...●●...xxxx
    1 ..●●...●●...xxxx
    2 ..●●...●●...xxxx
    3 ..●●...●●...xxxx
    4 ..●●...●●...xxxx
    5 ..●●●●●●●...xxxx
    6 ...●●●●●●...xxxx
    7 .......●●...xxxx
    8 .......●●...xxxx
    9 ...●●●●●●...xxxx
    0 ...●●●●●....xxxx

    The matrix data would be
    |4 empty lines                            |, then the dot data
    {0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, 0x1F,0x00, 0x3F,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x31,0x80, 0x3F,0x80, 0x1F,0x80, 0x01,0x80, 0x01,0x80, 0x1F,0x80, 0x1F,0x00}

    */

class Font
{
public:
    Font(const char *fontname);
    ~Font();
    const char *FontName() { return &fontName[0]; };

    uint16_t BytesPerCell() { return  bytesPerCell; }; 
    uint8_t ColumnsPerCell() { return  columnsPerCell; }; 
    uint8_t RowsPerCell() { return  rowsPerCell; }; 
    uint8_t BytesPerCellRow() { return  bytesPerCellRow; }; 
    uint8_t CharSpacing() { return  charSpacing; }; 
    uint8_t LineSpacing() { return  lineSpacing; }; 
    uint8_t Descender() { return  descender; }; 

    /// \brief Char width with spacing
    uint8_t CharWidthWS() { return  columnsPerCell+charSpacing; }; 

    /// \brief Char height with spacing
    uint8_t CharHeightWS() { return  rowsPerCell+lineSpacing; }; 

    /// \brief Get cell data
    uint8_t *GetCell(char c);

    /// \brief Get char width
    uint8_t GetWidth(char c);

    /// \brief Get string width
    uint8_t GetWidth(char *s);

private:
    char fontName[16];
    uint16_t bytesPerCell;
    uint8_t columnsPerCell;
    uint8_t rowsPerCell;
    uint8_t bytesPerCellRow;
    uint8_t charSpacing;
    uint8_t lineSpacing;
    uint8_t descender;

    uint8_t * cellPtr[0x80-0x20];  // 0x20 - 0x7F
};

#endif
