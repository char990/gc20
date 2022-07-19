#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
using namespace std;

UciPasswd::UciPasswd()
{
    PATH = "/etc/config";
    PACKAGE = "UciPasswd";
}

UciPasswd::~UciPasswd()
{
}

void UciPasswd::LoadConfig()
{
    Ldebug(">>> Loading 'UciPasswd'");
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
                UserPasswd userpass;
                userpass.passwd = string(GetStr(uciSec, _Passwd));
                userpass.permission = GetInt(uciSec, _Permission, 0, 9);
                string name(uciSec->e.name);
                if (mapUserPass.find(name) != mapUserPass.end())
                { // exist
                    mapUserPass[name] = userpass;
                }
                else
                {
                    mapUserPass.emplace(name, userpass);
                }
            }
        }
    }
    Close();
    Dump();
}

void UciPasswd::Set(const char *user, const char *passwd, const int permission)
{
    struct uci_ptr pa = {
        .package = PACKAGE,
        .section = user,
        .option = _Passwd,
        .value = passwd};

    Open();
    int r = uci_set(ctx, &pa);
    if (r != UCI_OK)
    {
        throw std::runtime_error(FmtException("SetByPtr failed(return %d): %s.%s.%s=%s", r,
                                              ptrSecSave.package, ptrSecSave.section, ptrSecSave.option, ptrSecSave.value));
    }
    OptionSave(_Permission, permission);
    CommitCloseSectionForSave();

    UserPasswd userpass;
    userpass.passwd = string(passwd);
    userpass.permission = permission;
    string name(user);
    SECTION = name.c_str();
    if (mapUserPass.find(name) != mapUserPass.end())
    { // exist
        mapUserPass[name] = userpass;
    }
    else
    {
        mapUserPass.emplace(name, userpass);
    }
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
