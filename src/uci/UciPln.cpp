#include <cstdio>
#include <cstring>
#include <uci.h>
#include <module/Utils.h>
#include <module/MyDbg.h>
#include <uci/DbHelper.h>

using namespace Utils;

UciPln::UciPln()
{
}

UciPln::~UciPln()
{
}

void UciPln::LoadConfig()
{
    PrintDbg(DBG_LOG, ">>> Loading 'plans'\n");
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
    //Dump();
}

void UciPln::Dump()
{
    PrintDash();
    printf("%s/%s\n", PATH, PACKAGE);
    for (auto &m : plns)
    {
        if (m.micode != 0)
        {
            printf("\t%s\n", m.ToString().c_str());
        }
    }
}

uint16_t UciPln::ChkSum()
{
    return chksum;
}

bool UciPln::IsPlnDefined(uint8_t i)
{
    return (i == 0 || plns.at(i - 1).micode == 0) ? false : true;
}

Plan *UciPln::GetPln(uint8_t i)
{
    return IsPlnDefined(i) ? &plns.at(i - 1) : nullptr;
}

uint8_t UciPln::GetPlnRev(uint8_t i)
{
    return IsPlnDefined(i) ? plns.at(i - 1).plnRev : 0;
}

APP::ERROR UciPln::SetPln(uint8_t *buf, int len)
{
    Plan pln;
    APP::ERROR r = pln.Init(buf, len);
    if (r == APP::ERROR::AppNoError)
    {
        int i = pln.plnId - 1;
        if (plns.at(i).micode != 0)
        {
            chksum -= plns.at(i).crc;
        }
        plns.at(i) = pln;
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
    int len = plns.at(i).ToArray(a);
    Cnvt::ParseToStr(a, v, len);
    OpenSaveClose(SECTION, option, v);
}

void UciPln::Reset()
{
    Plan p;
    plns.fill(p);
    UciCfg::ClrSECTION();
}
