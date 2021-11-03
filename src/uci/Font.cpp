#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <uci/Font.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

Font::Font(const char *fontname)
{
    int fnlen = strlen(fontname);
    if(fnlen>8)
    {
        MyThrow("fontname(%s) too long", fontname);
    }
    memcpy(fontName, fontname, fnlen);
    fontName[fnlen]='\0';
    for (int i = 0x20; i < 0x80; i++)
    {
        cellPtr[i - 0x20] = nullptr;
    }

    char fn[16];
    sprintf(fn, "font/%s", fontName);
    int fd = open(fn, O_RDONLY);
    if (fd < 0)
    {
        MyThrow("Can't open file %s", fn);
    }
    uint8_t buf[MAX_FONT_DOT / 8 + 1];
    int n;
    n = read(fd, buf, 8);
    if (n != 8)
    {
        close(fd);
        MyThrow("Read file %s failed", fn);
    }
    uint8_t *p = buf;
    bytesPerCell = Utils::Cnvt::GetU16(p);
    p += 2;
    columnsPerCell = *p++;
    rowsPerCell = *p++;
    bytesPerCellRow = *p++;
    descender = *p++;
    charSpacing = *p++;
    lineSpacing = *p++;
    // assume the smallest font is 4*6, largest font is 64*64
    if (bytesPerCell != (1 + bytesPerCellRow * rowsPerCell) ||
        columnsPerCell < 4 || columnsPerCell > 64 ||
        rowsPerCell < 6 || rowsPerCell > 64 ||
        bytesPerCellRow != ((columnsPerCell + 7) / 8) ||
        charSpacing > 16 ||
        lineSpacing > 16 ||
        descender > lineSpacing)
    {
        close(fd);
        MyThrow("Corrupt data in file %s", fontName);
    }
    for (int i = 0x20; i < 0x80; i++)
    {
        n = read(fd, buf, bytesPerCell);
        if (n != bytesPerCell)
        {
            break;
        }
        cellPtr[i - 0x20] = new uint8_t[bytesPerCell];
        memcpy(cellPtr[i - 0x20], buf, bytesPerCell);
    }
    close(fd);
}

Font::~Font()
{
    for (int i = 0x20; i < 0x80; i++)
    {
        if (cellPtr[i - 0x20] != nullptr)
        {
            delete[] cellPtr[i - 0x20];
        }
    }
}

uint8_t *Font::GetCell(char c)
{
    if (c < 0x20 || c > 127 || c == 0x5F)
    {
        c = 0x20;
    }
    return cellPtr[c - 0x20];
}

uint8_t Font::GetWidth(char c)
{
    if (c < 0x20 || c > 127 || c == 0x5F)
    {
        c = 0x20;
    }
    uint8_t *cell = cellPtr[c - 0x20];
    return (*cell) & 0x7F;
}

uint8_t Font::GetWidth(char *s)
{
    int len = strlen(s);
    if (len == 0)
    {
        return 0;
    }
    int x = (len - 1) * charSpacing;
    for (int i = 0; i < len; i++)
    {
        x += GetWidth(*s++);
    }
    return x;
}
