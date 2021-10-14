#include <cstdio>
#include <cstring>
#include <uci.h>
#include <module/Utils.h>
#include <module/MyDbg.h>
#include <uci/DbHelper.h>

using namespace Utils;

UciPln::UciPln()
{
    plns = new Plan[255];
}

UciPln::~UciPln()
{
    delete[] plns;
}

void UciPln::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciPln";
    SECTION = "pln";
    chksum = 0;
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    struct uci_element *e;
    struct uci_option *option;
    Plan pln;
    uint8_t b[PLN_LEN_MAX + PLN_TAIL];
    int i;
    uci_foreach_element(&uciSec->options, e)
    {
        if (sscanf(e->name, _Option, &i) == 1)
        {
            option = uci_to_option(e);
            if (i >= 1 && i <= 255 && option->type == uci_option_type::UCI_TYPE_STRING)
            {
                int len = strlen(option->v.string);
                if (Cnvt::ParseToU8(option->v.string, b, len) == 0)
                {
                    len /= 2;
                    if (Crc::Crc16_1021(b, len - PLN_TAIL) == Cnvt::GetU16(b + len - PLN_TAIL))
                    {
                        SetPln(b, len);
                    }
                }
            }
        }
    }
    Close();
    Dump();
}

void UciPln::Dump()
{
    PrintDash();
    printf("%s/%s\n", PATH, PACKAGE);
    for (int i = 1; i <= 255; i++)
    {
        if (IsPlnDefined(i))
        {
            printf("\t%s\n", plns[i - 1].ToString().c_str());
        }
    }
}

uint16_t UciPln::ChkSum()
{
    return chksum;
}

bool UciPln::IsPlnDefined(uint8_t i)
{
    return (i == 0 || plns[i - 1].micode == 0) ? false : true;
}

Plan *UciPln::GetPln(uint8_t i)
{
    return IsPlnDefined(i) ? &plns[i - 1] : nullptr;
}

uint8_t UciPln::GetPlnRev(uint8_t i)
{
    return IsPlnDefined(i) ? plns[i - 1].plnRev : 0;
}

APP::ERROR UciPln::SetPln(uint8_t *buf, int len)
{
    Plan pln;
    APP::ERROR r = pln.Init(buf, len);
    if (r == APP::ERROR::AppNoError)
    {
        // check plan overlap
        int i = pln.plnId - 1;
        if (plns[i].micode != 0)
        {
            chksum -= plns[i].crc;
        }
        plns[i] = pln;
        chksum += pln.crc;
    }
    return r;
}

void UciPln::SavePln(uint8_t i)
{
    if (!IsPlnDefined(i))
        return;
    char option[8];
    sprintf(option, _Option, i);
    uint8_t a[PLN_LEN_MAX + PLN_TAIL];
    char v[(PLN_LEN_MAX + PLN_TAIL) * 2 + 1];
    i--;
    int len = plns[i].ToArray(a);
    Cnvt::ParseToStr(a, v, len);
    OpenSaveClose(SECTION, option, v);
}

void UciPln::Reset()
{
    for (int i = 0; i < 255; i++)
    {
        plns[i].micode = 0;
    }
    UciCfg::ClrSECTION();
}
