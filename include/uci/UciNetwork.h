#pragma once

#include <string>

#include <uci/UciCfg.h>
#include <module/Utils.h>

class NetInterface
{
public:
    std::string proto;
    uint8_t ipaddr[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    std::string dns;
};

class NtpServer
{
public:
    std::string server;
    uint16_t port;
};


class UciNetwork : public UciCfg
{
public:
    UciNetwork();
    ~UciNetwork();
    virtual void LoadConfig() override;
    virtual void Dump() override;

    // getter
    NetInterface & GetETH(int i) { return eth[i];};


    void Ipaddr(int i, uint8_t *ip);
    void Netmask(int i, uint8_t *ip);
    void Gateway(int i, uint8_t *ip);
    void Dns(int i, std::string s);

private:
    void LoadIp(struct uci_section *uciSec, const char *_option, uint8_t *dst);
    void PrintIp(const char *_option, uint8_t *dst);
    //void Set(uint8_t * src, const char * _option, uint8_t * dst);
    const char *_ETH = "ETH";
    const char *_proto = "proto";
    const char *_ipaddr = "ipaddr";
    const char *_netmask = "netmask";
    const char *_gateway = "gateway";
    const char *_dns = "dns";

    // NTP
    const char *_server = "server";
    const char *_port = "port";

    NetInterface eth[2];
    NtpServer ntp;

    void Set(int i, uint8_t *src, const char *_option, uint8_t *dst);

};
