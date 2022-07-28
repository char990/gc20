#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
using namespace std;

UciNetwork::UciNetwork()
{
    NetInterface init;
    eths.emplace(string("ETH1"), init);
    eths.emplace(string("ETH2"), init);
}

void UciNetwork::LoadConfig()
{
    PATH = "/etc/config";
    PACKAGE = "network";
    Ldebug(">>> Loading '%s/%s'", PATH, PACKAGE);
    auto LoadIp = [this](struct uci_section *uciSec, const char *_option, Ipv4 &ip, bool ex = true)
    {
        const char *str = GetStr(uciSec, _option, ex);
        if (ip.Set(str))
        {
            return;
        }
        if (ex == false)
        {
            ip.Set("0.0.0.0");
            return;
        }
        throw invalid_argument(StrFn::PrintfStr("%s/%s.%s.%s Error: %s", PATH, PACKAGE, SECTION, _option, str));
    };

    Open();
    for (auto &x : eths)
    {
        SECTION = x.first.c_str();
        struct uci_section *uciSec = GetSection(SECTION);
        auto &e = x.second;
        e.proto = string(GetStr(uciSec, _Proto));
        if (e.proto.compare("static") == 0)
        {
            LoadIp(uciSec, _Ipaddr, e.ipaddr);
            LoadIp(uciSec, _Netmask, e.netmask);
            LoadIp(uciSec, _Gateway, e.gateway, false);
        }
        const char *str = GetStr(uciSec, _Dns, false);
        if (str != nullptr)
        {
            e.dns = string(str);
        }
    }
    Close();
}

void UciNetwork::SaveETH(string name)
{
    NetInterface *n = GetETH(name);
    if (n != nullptr)
    {
        if (n->proto.compare("static") != 0)
        {
            n->ipaddr.Set("0.0.0.0");
            n->netmask.Set("0.0.0.0");
            n->gateway.Set("0.0.0.0");
        }
        OpenSectionForSave(name.c_str());
        OptionSave(_Proto, n->proto.c_str());
        OptionSave(_Ipaddr, n->ipaddr.ToString().c_str());
        OptionSave(_Netmask, n->netmask.ToString().c_str());
        OptionSave(_Gateway, n->gateway.ToString().c_str());
        OptionSave(_Dns, n->dns.c_str());
        CommitCloseSectionForSave();
    }
}

void UciNetwork::SaveNtp()
{
}

void UciNetwork::Dump()
{
    auto PrintIp = [](const char *_option, Ipv4 & dst)
    {
        printf("\t%s \t'%s'\n", _option, dst.ToString().c_str());
    };

    PrintDash('<');
    for (auto &x : eths)
    {
        printf("%s/%s.%s\n", PATH, PACKAGE, x.first.c_str());
        auto &e = x.second;
        printf("\t%s \t'%s'\n", _Proto, e.proto.c_str());
        if (e.proto.compare("static") == 0)
        {
            PrintIp(_Ipaddr, e.ipaddr);
            PrintIp(_Netmask, e.netmask);
            PrintIp(_Gateway, e.gateway);
        }
        printf("\t%s \t'%s'\n", _Dns, e.dns.c_str());
    }
    printf("NTP server: %s:%d\n", ntp.server.c_str(), ntp.port);
    PrintDash('>');
}
