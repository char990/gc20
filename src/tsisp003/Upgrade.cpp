#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <tsisp003/Upgrade.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

const char *GDIR = "goblin_temp";
const char *GFILE = "goblin";
const char *GMD5FILE = "goblin.md5";
const char *UFILE = "goblin.tar";

using namespace Utils;

#define FILE_SIZE_MIN (64 * 1024)
#define FILE_SIZE_MAX (4 * 1024 * 1024)

int Upgrade::FileInfo(uint8_t *data)
{
    uint32_t len = Cnvt::GetU32(data + 4);
    if (len < FILE_SIZE_MIN || len > FILE_SIZE_MAX)
    {
        Ldebug("Upgrade::FileInfo: len=%d", len);
        return 1;
    }
    uint8_t sizeK = data[8];
    if (sizeK != 1 && sizeK != 4 && sizeK != 16 && sizeK != 64)
    {
        Ldebug("Upgrade::FileInfo: PktSize=%d", sizeK);
        return 2;
    }
    char pf[PRINT_BUF_SIZE];
    snprintf(pf, PRINT_BUF_SIZE-1, "%s/%s", GDIR, UFILE);
    int fd = open(pf, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        Ldebug("Upgrade::FileInfo: Can't open '%s'", pf);
        return fd;
    }
    close(fd);
    filetype = data[2];
    config = data[3];
    filelen = len;
    pktsizeK = sizeK;
    totalPkt = (len + sizeK * 1024 - 1) / (sizeK * 1024);
    if (rcvd != nullptr)
    {
        delete[] rcvd;
    }
    rcvd = new uint8_t[totalPkt];
    Ldebug("Upgrade::FileInfo: success");
    return 0;
}

int Upgrade::FilePacket(uint8_t *data, int len)
{
    if (rcvd == nullptr || totalPkt == 0 || filelen == 0 || pktsizeK == 0)
    {
        Ldebug("Upgrade::FilePacket: Need file info");
        return 1;
    }
    pktN = Cnvt::GetU16(data + 2);
    len -= 4;
    if (pktN > (totalPkt - 1))
    {
        Ldebug("Upgrade::FilePacket: pktN[%d](0-%d)", pktN, totalPkt - 1);
        return 2;
    }
    else if (pktN < (totalPkt - 1))
    {
        if (len != (pktsizeK * 1024))
        {
            Ldebug("Upgrade::FilePacket: Pkt[%d] len=%d", pktN, len);
            return 3;
        }
    }
    else // if (pktN == (totalPkt - 1)) // last packet
    {
        if (len != (filelen % (pktsizeK * 1024)))
        {
            Ldebug("Upgrade::FilePacket: Pkt[%d] len=%d", pktN, len);
            return 4;
        }
    }
    char pf[PRINT_BUF_SIZE];
    snprintf(pf, PRINT_BUF_SIZE-1, "%s/%s", GDIR, UFILE);
    int fd = open(pf, O_WRONLY);
    if (fd < 0)
    {
        Ldebug("Upgrade::FilePacket: Can't open '%s'", pf);
        return fd;
    }
    int wr;
    wr = lseek(fd, pktN * pktsizeK * 1024, SEEK_SET);
    if (wr < 0)
    {
        Ldebug("Upgrade::FilePacket: lseek failed:pkt[%d]size[%d]", pktN, pktsizeK);
    }
    else
    {
        wr = write(fd, data + 4, len);
        if (wr == len)
        {
            rcvd[pktN] = 1;
            wr = 0;
        }
        else
        {
            Ldebug("Upgrade::FilePacket: Write '%s' failed:wr[%d]len[%d]", pf, wr, len);
            wr = -1;
        }
    }
    close(fd);
    return wr;
}

int Upgrade::Start()
{
    if (rcvd == nullptr || totalPkt == 0 || filelen == 0 || pktsizeK == 0)
    {
        Ldebug("Upgrade::Start: Need file info");
        return 1;
    }
    for (int i = 0; i < totalPkt; i++)
    {
        if (rcvd[pktN] == 0)
        {
            Ldebug("Upgrade::Start: Receiving file uncompleted");
            return 2;
        }
    }

    BackupFirmware();

    char buf[PRINT_BUF_SIZE];
    int r = UnpackFirmware(buf, md5);
    if (r != 0)
    {
        memset(md5, 0, 33);
    }
    Ldebug(buf);
    filelen = 0;
    pktsizeK = 0;
    totalPkt = 0;
    delete[] rcvd;
    rcvd = nullptr;
    pktN = 0;
    return r;
}

void Upgrade::RemoveAllTempFiles()
{
    remove(GFILE);
    remove(GMD5FILE);
    remove(UFILE);
}

void Upgrade::BackupFirmware()
{
    Exec::Shell("md5sum %s > %s", GFILE, GMD5FILE);
    Exec::Shell("tar -cf %s %s %s", UFILE, GFILE, GMD5FILE);
}

int Upgrade::UnpackFirmware(char *buf, char *md5)
{
    if (chdir(GDIR) != 0)
    {
        snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: Can't change path to '%s'", GDIR);
        return -1;
    }
    int r = 0;
    try
    {
        if (Exec::Shell("tar -xf %s", UFILE) != 0)
        {
            snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: Fail to unpack '%s'", UFILE);
            r = 4;
        }
        else
        {
            if (!Exec::FileExists(GFILE))
            {
                snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: Can't find '%s'", GFILE);
                r = 6;
            }
            else if (!Exec::FileExists(GMD5FILE))
            {
                snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: Can't find '%s'", GMD5FILE);
                r = 7;
            }
            else
            {
                if (Exec::Shell(TO_NULL("md5sum -c %s"), GMD5FILE) != 0)
                {
                    snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: MD5 NOT matched");
                    r = 8;
                }
                else
                {
                    int md5f = open(GMD5FILE, O_RDONLY);
                    if (md5f > 0 && read(md5f, md5, 32) == 32)
                    {
                        md5[32] = '\0';
                        snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: MD5=%s. Restart...",md5);
                    }
                    else
                    {
                        snprintf(buf, PRINT_BUF_SIZE-1, "Upgrade: Can't get MD5 from '%s'", GMD5FILE);
                        r = 9;
                    }
                    close(md5f);
                }
            }
        }
    }
    catch (std::exception &e)
    {
        r = -1;
        snprintf(buf, PRINT_BUF_SIZE-1, "%s", e.what());
    }
    chdir("..");
    return r;
}
