#include <sign/FrameImage.h>
#include <uci/DbHelper.h>
#include <3rdparty/mongoose/mongoose.h>
#include <tsisp003/TsiSp003Const.h>

FrameImage::FrameImage(uint8_t signId, uint8_t frameId)
    : signId(signId), frameId(frameId)
{
}

/*
    enum SLVCMD
    {
        RQST_STATUS = 0x05,
        RPL_STATUS = 0x06,
        RQST_EXT_ST = 0x07,
        RPL_EXT_ST = 0x08,
        SYNC = 0x09,
        SET_TXT_FRM = 0x0A,
        SET_GFX_FRM = 0x0B,
        DISPLAY_FRM = 0x0E,
        SET_STD_FRM = 0x0F,
        SET_ISLUS_SP_FRM = 0xFC
    };
*/

void FrameImage::FillCore(uint8_t *frame, int len)
{
    if (frame[1] == 0xFC)
    {
        if (frame[6] == 189)
        {
            bmp.ReadFromFile("config/islus_189.bmp");
        }
        else if (frame[6] == 199)
        {
            bmp.ReadFromFile("config/islus_199.bmp");
        }
        else if (frame[6] == 251)
        {
            bmp.ReadFromFile("config/islus_251.bmp");
        }
        else
        {
            bmp.ReadFromFile("config/annulus_off.bmp");
        }
        return;
    }
    if (frame[1] != 0x0B)
    {
        bmp.ReadFromFile("config/annulus_off.bmp");
        return;
    }
    int annulus = (frame[7] >> 3) & 0x03;
    if (annulus == 3)
        annulus = 0;
    bmp.ReadFromFile(annulus ? "config/annulus_on.bmp" : "config/annulus_off.bmp");
    int lantern = frame[7] & 0x07;
    if (lantern > 5)
        lantern = 0;
    if (lantern > 0)
    {
        // TODO: set lantern
    }
    auto &prod = DbHelper::Instance().GetUciProd();
    int coreOffsetX = prod.CoreOffsetX();
    int coreOffsetY = prod.CoreOffsetY();
    int coreRows = prod.PixelRows();
    int coreColumns = prod.PixelColumns();
    if (frame[6] >= 0 && frame[6] <= 9)
    {
        // TODO: mono
    }
    else if (frame[6] == 0x0D)
    {
        // TODO: 4-bit
    }
    else if (frame[6] == 0x0E)
    {
    }
    else if (frame[6] == 0xF1)
    {
    }
    else
    {
    }
    char filename[256];
    sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, frameId);
    bmp.WriteToFile(filename);
}

void FrameImage::Save2Base64(std::vector<char> &base64)
{
    char filename[256];
    sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, frameId);

    int fd;
    struct stat statbuf;

    fd = open(filename, O_RDONLY);
    if (fd == -1 || fstat(fd, &statbuf) == -1)
    {
        return;
    }
    int len = statbuf.st_size;
    unsigned char *filebuf = new unsigned char[len];
    unsigned char *p = filebuf;
    base64.resize((len + 2) / 3 * 4 + 1);
    int n;
    int slen = 0;
    while ((n = read(fd, p, 4096)) > 0)
    {
        p += n;
        slen += n;
    }
    close(fd);
    int base64len = mg_base64_encode(filebuf, len, base64.data());
    base64.back() = '\0';
    delete filebuf;
}
