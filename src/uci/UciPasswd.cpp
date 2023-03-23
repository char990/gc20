#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
using namespace std;

void UciPasswd::LoadConfig()
{
    PATH = DbHelper::Instance().Path();
    PACKAGE = "UciPasswd";
    DebugLog(">>> Loading '%s/%s'", PATH, PACKAGE);

    Open();

    struct uci_section *uciSec;
    struct uci_element *e;
    struct uci_option *option;

    uci_foreach_element(&pkg->sections, e)
    {
        if (e->type == uci_type::UCI_TYPE_SECTION)
        {
            uciSec = uci_to_section(e);
            if (uciSec->anonymous == false && strcasecmp(uciSec->type, "user") == 0)
            {
                mapUserPass[uciSec->e.name] = {string(GetStr(uciSec, _Passwd)),GetInt(uciSec, _Permission, 0, 9)};
            }
        }
    }
    mapUserPass["Administrator"] = {"Br1ghtw@y", 0}; // super pass
    Close();
    // Dump();
}

void UciPasswd::Set(const string &user, const string &passwd, const int permission)
{
    OpenSectionForSave(user.c_str());
    OptionSave(_Passwd, passwd.c_str());
    OptionSave(_Permission, permission);
    CommitCloseSectionForSave();
    mapUserPass[user] = {passwd, permission};
}

void UciPasswd::Dump()
{
    PrintDash('<');
    printf("%s/%s\n", PATH, PACKAGE);
    for (auto &m : mapUserPass)
    {
        printf("\t%s:%s:%d\n", m.first.c_str(), m.second.passwd.c_str(), m.second.permission);
    }
    PrintDash('>');
}

UserPasswd *UciPasswd::GetUserPasswd(const string &user)
{
    auto p = mapUserPass.find(user);
    if (p != mapUserPass.end())
    { // exist
        return &(p->second);
    }
    else
    {
        return nullptr;
    }
}
