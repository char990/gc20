#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <uci/Font.h>
#include <module/Utils.h>
#include <module/MyDbg.h>
#include <module/Utils.h>

using namespace Utils;
using namespace std;


Font::Font(const char *fontname)
{
    cellPtr.fill(nullptr);
    int fnlen = strlen(fontname);
    if(fnlen>8)
    {
        throw invalid_argument(StrFn::PrintfStr("fontname(%s) too long", fontname));
    }
    memcpy(fontName, fontname, fnlen);
    fontName[fnlen]='\0';
    char fn[16];
    snprintf(fn, 15, "font/%s", fontName);
    int fd = open(fn, O_RDONLY);
    if (fd < 0)
    {
        throw runtime_error(StrFn::PrintfStr("Can't open file %s", fn));
    }
    uint8_t buf[MAX_FONT_DOT / 8 + 1];
    int n;
    n = read(fd, buf, 8);
    if (n != 8)
    {
        close(fd);
        throw runtime_error(StrFn::PrintfStr("Read file %s failed", fn));
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
    // smallest font is 4*6, largest font is 32*64
    if (bytesPerCell != (1 + bytesPerCellRow * rowsPerCell) ||
        columnsPerCell < 4 || columnsPerCell > 32 ||
        rowsPerCell < 6 || rowsPerCell > 64 ||
        bytesPerCellRow != ((columnsPerCell + 7) / 8) ||
        charSpacing > 16 ||
        lineSpacing > 16 ||
        descender > lineSpacing)
    {
        close(fd);
        throw runtime_error(StrFn::PrintfStr("Corrupt data in file %s", fontName));
    }
    for (int i = 0; i < cellPtr.size(); i++)
    {
        cellPtr.at(i) = new uint8_t[bytesPerCell];
        n = read(fd, cellPtr.at(i), bytesPerCell);
        if (n != bytesPerCell)
        {
            close(fd);
            throw runtime_error(StrFn::PrintfStr("Corrupt data in file %s", fontName));
        }
    }
    close(fd);
}

Font::~Font()
{
    for (auto & c : cellPtr)
    {
        if (c != nullptr)
        {
            delete[] c;
        }
    }
}

uint8_t *Font::GetCell(char c)
{
    if (c < 0x20 || c > 127 || c == 0x5F)
    {
        c = 0x20;
    }
    return cellPtr.at(c - 0x20);
}

int Font::GetWidth(char c)
{
    if (c < 0x20 || c > 127 || c == 0x5F)
    {
        c = 0x20;
    }
    uint8_t *cell = cellPtr.at(c - 0x20);
    return (*cell) & 0x7F;
}

int Font::GetWidth(const char *s)
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
