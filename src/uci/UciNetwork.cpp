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
    Dump();
}

int UciNetwork::UciSet(const char *section, const char *option, string str)
{
    return Exec::Shell("uci -c %s set %s.%s.%s=%s", PATH, PACKAGE, section, option, str.c_str());
}

int UciNetwork::SaveETH(string name, NetInterface &nif)
{
    auto eth = GetETH(name);
    if (eth == nullptr)
    {
        throw invalid_argument("Unknown interface name. Should be ETH1/2");
    }
    if (nif.proto.compare("static") != 0 && nif.proto.compare("dhcp") != 0)
    {
        throw invalid_argument("Unknown proto. Should be static/dhcp");
    }
    int r;
    const char *sec = name.c_str();
    if (eth->proto.compare(nif.proto) != 0)
    {
        r = UciSet(sec, _Proto, nif.proto);
        if (r != 0)
        {
            return r;
        }
        evts.push_back("Set " + name + ".proto=" + nif.proto);
        eth->proto = nif.proto;
    }
    if (eth->proto.compare("static") == 0)
    {
        if (eth->ipaddr.Compare(nif.ipaddr) != 0)
        {
            r = UciSet(sec, _Ipaddr, nif.ipaddr.ToString());
            if (r != 0)
            {
                return r;
            }
            evts.push_back("Set " + name + ".ipaddr=" + nif.ipaddr.ToString());
            eth->ipaddr = nif.ipaddr;
        }
        if (eth->netmask.Compare(nif.netmask) != 0)
        {
            r = UciSet(sec, _Netmask, nif.netmask.ToString());
            if (r != 0)
            {
                return r;
            }
            evts.push_back("Set " + name + ".netmask=" + nif.netmask.ToString());
            eth->netmask = nif.netmask;
        }
        if (eth->gateway.Compare(nif.gateway) != 0)
        {
            r = UciSet(sec, _Gateway, nif.gateway.ToString());
            if (r != 0)
            {
                return r;
            }
            evts.push_back("Set " + name + ".gateway=" + nif.gateway.ToString());
            eth->gateway = nif.gateway;
        }
        if (eth->dns.compare(nif.dns) != 0)
        {
            r = UciSet(sec, _Dns, nif.dns);
            if (r != 0)
            {
                return r;
            }
            evts.push_back("Set " + name + ".dns=" + nif.dns);
            eth->dns = nif.dns;
        }
    }
    else
    {
        UciSet(sec, _Ipaddr, string(""));
        UciSet(sec, _Netmask, string(""));
        UciSet(sec, _Gateway, string(""));
        UciSet(sec, _Dns, string(""));
    }
    return 0;
}

int UciNetwork::SaveNtp(NtpServer &ntps)
{
    if (ntp.server.compare(ntps.server) != 0)
    {
        evts.push_back("Set NTP.server=" + ntps.server);
        ntp.server = ntps.server;
    }
    if (ntp.port != ntps.port)
    {
        evts.push_back("Set NTP.port=" + to_string(ntps.port));
        ntp.port = ntps.port;
    }
    return 0;
}

int UciNetwork::UciCommit()
{
    int r = system("uci -c /etc/config commit");
    if(r==0)
    {
        auto &evlog = DbHelper::Instance().GetUciEvent();
        for (auto e : evts)
        {
            evlog.Push(0, e.c_str());
        }
        evts.clear();
    }
    return r;
}

void UciNetwork::Dump()
{
    auto PrintIp = [](const char *_option, Ipv4 &dst)
    {
        printf("\t%s \t'%s'\n", _option, dst.ToString().c_str());
    };

    PrintDash('<');
    for (auto &x : eths)
    {
        printf("%s\n", x.first.c_str());
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
