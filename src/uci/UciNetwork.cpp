#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

UciNetwork::UciNetwork()
{
    PATH = "/etc/config";
    PACKAGE = "network";
}

UciNetwork::~UciNetwork()
{
}

void UciNetwork::LoadConfig()
{
    Ldebug(">>> Loading 'network'");
    Open();
    char ethx[5];
    for (int i = 0; i < 2; i++)
    {
        sprintf(ethx, "%s%d", _ETH, i + 1);
        SECTION = ethx;
        struct uci_section *uciSec = GetSection(SECTION);
        eth[i].proto = std::string(GetStr(uciSec, _proto));
        if (eth[i].proto.compare("static") == 0)
        {
            LoadIp(uciSec, _ipaddr, eth[i].ipaddr);
            LoadIp(uciSec, _netmask, eth[i].netmask);
            LoadIp(uciSec, _gateway, eth[i].gateway);
        }
        const char *str = GetStr(uciSec, _dns, false);
        if (str != nullptr)
        {
            eth[i].dns = std::string(str);
        }
    }
    Close();
}

void UciNetwork::LoadIp(struct uci_section *uciSec, const char *_option, uint8_t *dst)
{
    const char *str = GetStr(uciSec, _option);
    if (str != nullptr)
    {
        int ibuf[4];
        int cnt = Cnvt::GetIntArray(str, 4, ibuf, 0, 255);
        if (cnt == 4)
        {
            for (int i = 0; i < 4; i++)
            {
                dst[i] = ibuf[i];
            }
            return;
        }
    }
    throw std::invalid_argument(FmtException("%s/%s.%s.%s Error: %s", PATH, PACKAGE, SECTION, _option, str));
}

void UciNetwork::Ipaddr(int i, uint8_t *ip)
{
    Set(i, ip, _ipaddr, eth[i].ipaddr);
}

void UciNetwork::Netmask(int i, uint8_t *ip)
{
    Set(i, ip, _netmask, eth[i].netmask);
}

void UciNetwork::Gateway(int i, uint8_t *ip)
{
    Set(i, ip, _gateway, eth[i].gateway);
}

void UciNetwork::Set(int i, uint8_t *src, const char *_option, uint8_t *dst)
{
    memcpy(dst, src, 4);
    char buf[16];
    sprintf(buf, "%d.%d.%d.%d", dst[0], dst[1], dst[2], dst[3]);
    char ethx[5];
    sprintf(ethx, "%s%d", _ETH, i + 1);
    SECTION = ethx;
    OpenSaveClose(SECTION, _option, buf);
}

void UciNetwork::Dns(int i, std::string s)
{
}

void UciNetwork::Dump()
{
    PrintDash('<');
    for (int i = 0; i < 2; i++)
    {
        printf("%s/%s.%s%d\n", PATH, PACKAGE, _ETH, i + 1);
        printf("\t%s \t'%s'\n", _proto, eth[i].proto.c_str());
        if (eth[i].proto.compare("static") == 0)
        {
            PrintIp(_ipaddr, eth[i].ipaddr);
            PrintIp(_netmask, eth[i].netmask);
            PrintIp(_gateway, eth[i].gateway);
        }
        printf("\t%s \t'%s'\n", _dns, eth[i].dns.c_str());
    }
    printf("NTP server: %s:%d\n", ntp.server.c_str(), ntp.port);
    PrintDash('>');
}

void UciNetwork::PrintIp(const char *_option, uint8_t *dst)
{
    printf("\t%s \t'%d.%d.%d.%d'\n", _option, dst[0], dst[1], dst[2], dst[3]);
}
