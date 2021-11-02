#pragma once

#include <string>

#include <uci/UciCfg.h>
#include <module/Utils.h>

class UciNetwork : public UciCfg
{
public:
    UciNetwork();
    ~UciNetwork();
    virtual void LoadConfig() override;
    virtual void Dump() override;

    // getter
    uint8_t *Ipaddr() { return ipaddr; };
    uint8_t *Netmask() { return netmask; };
    uint8_t *Gateway() { return gateway; };

    // setter
    void Ipaddr(uint8_t *);
    void Netmask(uint8_t *);
    void Gateway(uint8_t *);

private:
    void LoadIp(struct uci_section *uciSec, const char *_option, uint8_t *dst);
    void PrintIp(const char *_option, uint8_t *dst);
    void Set(uint8_t * src, const char * _option, uint8_t * dst);
    const char *_ipaddr = "ipaddr";
    const char *_netmask = "netmask";
    const char *_gateway = "gateway";

    uint8_t ipaddr[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
};
