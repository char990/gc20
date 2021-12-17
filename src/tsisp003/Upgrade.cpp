#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <tsisp003/Upgrade.h>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

#define FILE_SIZE_MIN (64 * 1024)
#define FILE_SIZE_MAX (4 * 1024 * 1024)

int Upgrade::FileInfo(uint8_t *data)
{
    uint32_t len = Cnvt::GetU32(data + 4);
    if (len < FILE_SIZE_MIN || len > FILE_SIZE_MAX)
    {
        PrintDbg(DBG_LOG, "Upgrade::FileInfo: len=%d", len);
        return 1;
    }
    uint8_t sizeK = data[8];
    if (sizeK != 1 && sizeK != 4 && sizeK != 16 && sizeK != 64)
    {
        PrintDbg(DBG_LOG, "Upgrade::FileInfo: PktSize=%d", sizeK);
        return 2;
    }
    char pf[256];
    snprintf(pf, 255, "%s/%s", GDIR, UFILE);
    int fd = open(pf, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        PrintDbg(DBG_LOG, "Upgrade::FileInfo: Can't open '%s'",pf);
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
    PrintDbg(DBG_LOG, "Upgrade::FileInfo: success");
    return 0;
}

int Upgrade::FilePacket(uint8_t *data, int len)
{
    if (rcvd == nullptr || totalPkt == 0 || filelen == 0 || pktsizeK == 0)
    {
        PrintDbg(DBG_LOG, "Upgrade::FilePacket: Need file info");
        return 1;
    }
    pktN = Cnvt::GetU16(data + 2);
    len -= 4;
    if (pktN > (totalPkt - 1))
    {
        PrintDbg(DBG_LOG, "Upgrade::FilePacket: pktN[%d](0-%d)", pktN, totalPkt - 1);
        return 2;
    }
    else if (pktN < (totalPkt - 1))
    {
        if (len != (pktsizeK * 1024))
        {
            PrintDbg(DBG_LOG, "Upgrade::FilePacket: Pkt[%d] len=%d", pktN, len);
            return 3;
        }
    }
    else // if (pktN == (totalPkt - 1)) // last packet
    {
        if (len != (filelen % (pktsizeK * 1024)))
        {
            PrintDbg(DBG_LOG, "Upgrade::FilePacket: Pkt[%d] len=%d", pktN, len);
            return 4;
        }
    }
    char pf[256];
    snprintf(pf, 255, "%s/%s", GDIR, UFILE);
    int fd = open(pf, O_WRONLY);
    if (fd < 0)
    {
        PrintDbg(DBG_LOG, "Upgrade::FilePacket: Can't open '%s'", pf);
        return fd;
    }
    int wr;
    wr = lseek(fd, pktN * pktsizeK * 1024, SEEK_SET);
    if (wr < 0)
    {
        PrintDbg(DBG_LOG, "Upgrade::FilePacket: lseek failed:pkt[%d]size[%d]", pktN, pktsizeK);
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
            PrintDbg(DBG_LOG, "Upgrade::FilePacket: Write '%s' failed:wr[%d]len[%d]", pf, wr, len);
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
        PrintDbg(DBG_LOG, "Upgrade::Start: Need file info");
        return 1;
    }
    for (int i = 0; i < totalPkt; i++)
    {
        if (rcvd[pktN] == 0)
        {
            PrintDbg(DBG_LOG, "Upgrade::Start: Receiving file uncompleted");
            return 2;
        }
    }
    char cwd[256];
    if (getcwd(cwd, 255) == nullptr)
    {
        PrintDbg(DBG_LOG, "Upgrade::CheckMD5: path is longer than 256 bytes.");
        return 3;
    }
    if (chdir(GDIR) != 0)
    {
        PrintDbg(DBG_LOG, "Upgrade::CheckMD5: Can't change path to '%s'", GDIR);
        return 4;
    }
    char cmd[256];
    snprintf(cmd, 255, "unzip -o -q %s", UFILE);
    int r = system(cmd);
    if (r != 0)
    {
        PrintDbg(DBG_LOG, "Upgrade::Start: Fail to unzip '%s'", UFILE);
        r = 5;
    }
    else
    {
        if (!Exec::FileExists(GFILE))
        {
            PrintDbg(DBG_LOG, "Upgrade::Start: Can't find '%s'", GFILE);
            r = 6;
        }
        else if (!Exec::FileExists(GMD5FILE))
        {
            PrintDbg(DBG_LOG, "Upgrade::Start: Can't find '%s'", GMD5FILE);
            r = 7;
        }
        else
        {
            PrintDbg(DBG_LOG, "Upgrade::Start: unzip '%s' success", UFILE);
            snprintf(cmd, 255, "md5sum -c %s", GMD5FILE);
            r = system(cmd);
            if (r != 0)
            {
                PrintDbg(DBG_LOG, "Upgrade::Start: MD5 NOT matched");
                r = 8;
            }
            else
            {
                PrintDbg(DBG_LOG, "Upgrade::Start: MD5 success");
                int md5f = open(GMD5FILE, O_RDONLY);
                if(md5f)
                {
                    read(md5f, md5, 32);
                    md5[32]='\0';
                    close(md5f);
                }
            }
        }
    }
    chdir(cwd);
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
