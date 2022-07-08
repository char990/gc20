#include <sign/FrameImage.h>
#include <uci/DbHelper.h>
#include <3rdparty/mongoose/mongoose.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/Utils.h>

void FrameImage::SetId(uint8_t signId, uint8_t slaveFrmId)
{
    this->signId = signId;
    this->slaveFrmId = slaveFrmId;
}

void FrameImage::FillCore(uint8_t *frame)
{
    if (slaveFrmId == 0)
    {
        return;
    }
    newImg = true;
    char filename[256];
    sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, slaveFrmId);
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
        bmp.WriteToFile(filename);
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
    { // colour is mapped colour
        // TODO: mono
        int colour = FrameColour::GetRGB8(frame[6]);
        RGBApixel rgba;
        rgba.Alpha = 0;
        rgba.Red = (colour >> 16) & 0xFF;
        rgba.Green = (colour >> 8) & 0xFF;
        rgba.Blue = (colour)&0xFF;
        uint8_t *p = &frame[10];
        int bitOffset = 0;
        for (int j = coreOffsetY; j < (coreOffsetY + coreRows); j++)
        {
            for (int i = coreOffsetX; i < (coreOffsetX + coreColumns); i++)
            {
                int x = bitOffset / 8;
                int b = 1 << (bitOffset & 0x07);
                if (p[x] & b)
                {
                    bmp.SetPixel(i, j, rgba);
                }
                bitOffset++;
            }
        }
    }
    else if (frame[6] == 0x0D)
    {
        // TODO: 4-bit
        RGBApixel rgba[10];
        for (int i = 0; i < 10; i++)
        {
            int colour = FrameColour::GetRGB8(i);
            rgba[i].Alpha = 0;
            rgba[i].Red = (colour >> 16) & 0xFF;
            rgba[i].Green = (colour >> 8) & 0xFF;
            rgba[i].Blue = (colour)&0xFF;
        }
        uint8_t *p = &frame[10];
        int bitOffset = 0;
        for (int j = coreOffsetY; j < (coreOffsetY + coreRows); j++)
        {
            for (int i = coreOffsetX; i < (coreOffsetX + coreColumns); i++)
            {
                uint8_t b = p[bitOffset / 2];
                if (bitOffset & 1)
                {
                    b &= 0x0F;
                }
                else
                {
                    b >>= 4;
                }
                if (b > 9)
                {
                    b = 0;
                }
                bmp.SetPixel(i, j, rgba[b]);
                bitOffset++;
            }
        }
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
    bmp.WriteToFile(filename);
}

std::vector<char> &FrameImage::Save2Base64()
{
    if (newImg)
    {
        char filename[256];
        if (slaveFrmId == 0)
        {
            sprintf(filename, "config/annulus_off.bmp");
        }
        else
        {
            sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, slaveFrmId);
        }

        int fd;
        struct stat statbuf;

        fd = open(filename, O_RDONLY);
        if (fd == -1 || fstat(fd, &statbuf) == -1)
        {
            return base64Img;
        }
        int len = statbuf.st_size;
        unsigned char *filebuf = new unsigned char[len];
        unsigned char *p = filebuf;
        int n;
        int slen = 0;
        while ((n = read(fd, p, 4096)) > 0)
        {
            p += n;
            slen += n;
        }
        close(fd);

        base64Img.resize((len + 2) / 3 * 4 + 1);
        mg_base64_encode(filebuf, len, base64Img.data());
        base64Img.back() = '\0';
        delete filebuf;
        newImg = false;
    }
    return base64Img;
}
