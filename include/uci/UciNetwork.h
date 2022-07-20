#pragma once

#include <string>
#include <map>

#include <uci/UciCfg.h>
#include <module/Utils.h>

class Ipv4
{
public:
    Ipv4(){};
    Ipv4(const uint8_t *p) { Set(p); };
    Ipv4(const std::string &str) { Set(str); }
    Ipv4(const char *str) { Set(str); }

    uint8_t ip[4]{0, 0, 0, 0};
    void Set(const uint8_t *p)
    {
        memcpy(ip, p, 4);
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
                    ip[i] = ipb[i];
                }
                return true;
            }
        }
        return false;
    }

    int Compare(Ipv4 &_ip)
    {
        return memcmp(ip, _ip.ip, 4);
    }

    bool Isvalid()
    {
        return !(ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0);
    }

    std::string ToString()
    {
        char buf[16];
        if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)
        {
            buf[0] = 0;
        }
        else
        {
            sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        }
        return std::string(buf);
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
    std::string server{"0.au.pool.ntp.org"};
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
    void SaveETH(std::string name);
    void SaveNtp();

    const char *_ETH = "ETH";
    const char *_Proto = "proto";
    const char *_Ipaddr = "ipaddr";
    const char *_Netmask = "netmask";
    const char *_Gateway = "gateway";
    const char *_Dns = "dns";

    // NTP
    const char *_Server = "server";
    const char *_Port = "port";

private:
    // void Set(uint8_t * src, const char * _option, uint8_t * dst);
    std::map<std::string, NetInterface> eths;
    NtpServer ntp;
};
