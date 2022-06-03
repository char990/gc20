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
    SECTION = "ETH1";
}

UciNetwork::~UciNetwork()
{
}

void UciNetwork::LoadConfig()
{
    Ldebug(">>> Loading 'network'");
    Open();
    struct uci_section *uciSec = GetSection(SECTION);
    LoadIp(uciSec, _ipaddr, ipaddr);
    LoadIp(uciSec, _netmask, netmask);
    LoadIp(uciSec, _gateway, gateway);
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

void UciNetwork::Ipaddr(uint8_t * ip)
{
    Set(ip, _ipaddr, ipaddr);
}

void UciNetwork::Netmask(uint8_t * ip)
{
    Set(ip, _netmask, netmask);
}

void UciNetwork::Gateway(uint8_t * ip)
{
    Set(ip, _gateway, gateway);
}

void UciNetwork::Set(uint8_t * src, const char * _option, uint8_t * dst)
{
    memcpy(dst, src, 4);
    char buf[16];
    sprintf(buf, "%d.%d.%d.%d", dst[0], dst[1], dst[2], dst[3]);
    OpenSaveClose(SECTION, _option, buf);
}

void UciNetwork::Dump()
{
    PrintDash('<');
    printf("%s/%s.%s\n", PATH, PACKAGE, SECTION);
    PrintIp(_ipaddr, ipaddr);
    PrintIp(_netmask, netmask);
    PrintIp(_gateway, gateway);
	PrintDash('>');
}

void UciNetwork::PrintIp(const char *_option, uint8_t *dst)
{
    printf("\t%s \t'%d.%d.%d.%d'\n", _option, dst[0], dst[1], dst[2], dst[3]);
}

