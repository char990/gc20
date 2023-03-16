#pragma once

#include <string>
#include <map>
#include <vector>

#include <uci/UciCfg.h>
#include <module/Utils.h>

class Ipv4
{
public:
    Ipv4() { ip.ip32 = 0; };
    Ipv4(const uint8_t *p) { Set(p); };
    Ipv4(const std::string &str) { Set(str); }
    Ipv4(const char *str) { Set(str); }

    union ips
    {
        uint8_t ipa[4];
        uint32_t ip32;
    } ip;

    void Set(const uint8_t *p)
    {
        memcpy(ip.ipa, p, 4);
    }

    bool Set(const std::string &str)
    {
        return Set(str.c_str());
    }

    bool Set(const char *str)
    {
        if (str != nullptr)
        {
            int ipb[4];
            int cnt = sscanf(str, "%d.%d.%d.%d", &ipb[0], &ipb[1], &ipb[2], &ipb[3]);
            if (cnt == 4)
            {
                for (int i = 0; i < 4; i++)
                {
                    if (ipb[i] < 0 || ipb[i] > 255)
                    {
                        return false;
                    }
                }
                for (int i = 0; i < 4; i++)
                {
                    ip.ipa[i] = ipb[i];
                }
                return true;
            }
        }
        return false;
    }

    int Compare(Ipv4 &_ip)
    {
        return memcmp(ip.ipa, _ip.ip.ipa, 4);
    }

    bool Isvalid()
    {
        return (ip.ip32 != 0);
    }

    std::string ToString()
    {
        if (ip.ip32 == 0)
        {
            return std::string("");
        }
        else
        {
            char buf[16];
            sprintf(buf, "%d.%d.%d.%d", ip.ipa[0], ip.ipa[1], ip.ipa[2], ip.ipa[3]);
            return std::string(buf);
        }
    };
};

class NetInterface
{
public:
    std::string proto;
    Ipv4 ipaddr;
    Ipv4 netmask;
    Ipv4 gateway;
    std::string dns{"8.8.8.8"};
};

class NtpServer
{
public:
    std::string server{"NTP disabled"};
    uint16_t port{123};
};

class UciNetwork : public UciCfg
{
public:
    UciNetwork();
    virtual void LoadConfig() override;
    virtual void Dump() override;

    // getter
    NetInterface *GetETH(std::string name)
    {
        auto p = eths.find(name);
        if (p != eths.end())
        { // exist
            return &(p->second);
        }
        else
        {
            return nullptr;
        }
    };
    NtpServer *GetNtp() { return &ntp; };

    // setter
    int UciSet(const char *section, const char *option, std::string str);
    int SaveETH(std::string name, NetInterface &nif);
    int SaveNtp(NtpServer &ntps);
    int UciCommit();

    const char *_ETH = "ETH";
    const char *_Proto = "proto";
    const char *_Ipaddr = "ipaddr";
    const char *_Netmask = "netmask";
    const char *_Gateway = "gateway";
    const char *_Dns = "dns";

    // NTP
    const char *_Server = "server";
    const char *_Port = "port";

    std::vector<std::string> evts;

private:
    std::map<std::string, NetInterface> eths;
    NtpServer ntp;
};
