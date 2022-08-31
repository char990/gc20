#pragma once

#include <cstdint>

extern const char *GDIR;
extern const char *GFILE;
extern const char *GMD5FILE;
extern const char *UFILE;

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

    void RemoveAllTempFiles();
    char md5[33]{0};
};

extern int UnpackFirmware(char *md5, char *buf);
