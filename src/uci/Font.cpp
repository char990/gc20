#include <unistd.h>
#include <fcntl.h>
#include <uci/Font.h>
#include <module/Utils.h>

Font::Font(const char *fontname)
{
    strncpy(fontName, fontname, 15);
    for (int i = 0; i < (0x7F - 0x20); i++)
    {
        cellPtr[i] = nullptr;
    }
    uint8_t buf[MAX_FONT_DOT / 8+1];
    sprintf(buf, "font/%s", fontName);
    int fd = open((const char *)buf, O_RDONLY);
    if (fd < 0)
    {
        MyThrow("Can't open file %s", fontName);
    }
    int n;
    n = read(buf, 8);
    if (n != 8)
    {
        close(fd);
        MyThrow("Read file %s failed", fontName);
    }
    bytesPerCell = buf[0] * 0x100 + buf[1];
    columnsPerCell = buf[2];
    rowsPerCell = buf[3];
    bytesPerCellRow = buf[4];
    charSpacing = buf[5];
    lineSpacing = buf[6];
    descender = buf[7];
    // assume the smallest font is 4*6, largest font is 64*64
    if (bytesPerCell != (1 + bytesPerCellRow * rowsPerCell) ||
        columnsPerCell < 4 || columnsPerCell > 64 ||
        rowsPerCell < 6 || rowsPerCell > 64 ||
        bytesPerCellRow != ((columnsPerCell + 7) / 8) ||
        charSpacing < 1 || charSpacing > 16 ||
        lineSpacing < 1 || lineSpacing > 16 ||
        descender < 1 || descender > lineSpacing)
    {
        close(fd);
        MyThrow("Corrupt data in file %s", fontName);
    }
    for (int i = 0; i < (0x7F - 0x20); i++)
    {
        n = read(buf, bytesPerCell);
        if (n != bytesPerCell)
        {
            break;
        }
        cellPtr[i] = new uint8_t[bytesPerCell];
        memcpy(cellPtr[i], buf, bytesPerCell);
    }
    close(fd);
}

Font::~Font()
{
    for (int i = 0; i < (0x7F - 0x20); i++)
    {
        if (cellPtr[i] != nullptr)
        {
            delete[] cellPtr[i];
        }
    }
}

uint8_t *Font::GetCell(char c)
{
    if (c < 0x20 || c >= 127 || c == 0x5F)
        c = 0x20;
    return cellPtr[c-0x20];
}

uint8_t Font::GetWidth(char c)
{
    if (c < 0x20 || c >= 127 || c == 0x5F)
        c = 0x20;
    uint8_t *cell = cellPtr[c - 0x20];
    return (*cell) & 0x7F;
}

uint8_t Font::GetWidth(char *s)
{
    int len = strlen(s)
    if (len == 0)
        return 0;
    int x = (len-1)*charSpacing;
    for(int i=0;i<len;i++)
    {
        x += GetWidth(*s++);
    }
    return x;
}



