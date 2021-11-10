#pragma once

#include <cstdint>

class Upgrade
{
public:
    int FileInfo(uint8_t *data);
    int FilePacket(uint8_t *data, int len);
    int Start();

    // getter
    uint16_t PktN() { return pktN; };
    uint16_t TotalPkt() { return totalPkt; };
    const char *MD5() { return md5; };

private:
    uint8_t filetype;
    uint8_t config;
    uint32_t filelen{0};
    uint8_t pktsizeK{0};
    uint16_t totalPkt{0};
    uint8_t *rcvd{nullptr};

    uint16_t pktN;

    const char *GDIR = "goblin_temp";
    const char *GFILE = "goblin";
    const char *GMD5FILE = "goblin.md5";
    const char *UFILE = "goblin.zip";
    void RemoveAllTempFiles();
    char md5[33]{0};
};
