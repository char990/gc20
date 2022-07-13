#include <sign/FrameImage.h>
#include <uci/DbHelper.h>
#include <3rdparty/mongoose/mongoose.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/Utils.h>

void FrameImage::SetId(uint8_t signId, uint8_t frmId)
{
    this->signId = signId;
    this->frmId = frmId;
}

void FrameImage::FillCore(uint8_t f_colour, uint8_t f_conspicuity, uint8_t *frame)
{
    int annulus = (f_conspicuity >> 3) & 0x03;
    if (annulus == 3)
        annulus = 0;
    bmp.ReadFromFile(annulus ? "config/annulus_on.bmp" : "config/annulus_off.bmp");
    int lantern = f_conspicuity & 0x07;
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
    if (f_colour >= 0 && f_colour <= 9)
    { // colour is mapped colour
        // TODO: mono
        int colour = FrameColour::GetRGB8(f_colour);
        RGBApixel rgba;
        rgba.Alpha = 0;
        rgba.Red = (colour >> 16) & 0xFF;
        rgba.Green = (colour >> 8) & 0xFF;
        rgba.Blue = (colour)&0xFF;
        uint8_t *p = frame;
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
    else if (f_colour == 0x0D)
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
        uint8_t *p = frame;
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
    else if (f_colour == 0x0E)
    {
    }
    else if (f_colour == 0xF1)
    {
    }
    else
    {
    }
}

void FrameImage::FillCoreFromSlaveFrame(uint8_t *frame)
{
    if (frmId == 0)
    {
        return;
    }
    newImg = true;
    char filename[256];
    sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, frmId);
    if (frame[1] == 0xFC)
    {
        if (DbHelper::Instance().GetUciProd().IsIslusSpFrm(frame[6]))
        {
            char islus_xxx[256];
            sprintf(islus_xxx, "config/islus_%03d.bmp", frame[6]);
            bmp.ReadFromFile(islus_xxx);
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
    FillCore(frame[6], frame[7], frame+10);
    bmp.WriteToFile(filename);
}

void FrameImage::FillCoreFromUciFrame()
{
    newImg = true;
    char filename[256];
    sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, frmId);
    if (DbHelper::Instance().GetUciProd().ProdType() == PRODUCT::ISLUS &&
        DbHelper::Instance().GetUciProd().IsIslusSpFrm(frmId))
    {
        char islus_xxx[256];
        sprintf(islus_xxx, "config/islus_%03d.bmp", frmId);
        bmp.ReadFromFile(islus_xxx);
        bmp.WriteToFile(filename);
        return;
    }
    auto frm = DbHelper::Instance().GetUciFrm().GetFrm(frmId);
    if(frm==nullptr)
    {
        return;
    }
    FillCore(frm->colour, frm->conspicuity, frm->stFrm.rawData.data() + frm->frmOffset);
    bmp.WriteToFile(filename);
}

std::vector<char> &FrameImage::Save2Base64()
{
    if (newImg)
    {
        char filename[256];
        if (frmId == 0)
        {
            sprintf(filename, "config/annulus_off.bmp");
        }
        else
        {
            sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, frmId);
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
void FrameImage::RemoveBmp()
{
    if (frmId == 0)
    {
        return;
    }
    newImg = false;
    char filename[256];
    sprintf(filename, "/tmp/Sign%d_Frm%d.bmp", signId, frmId);
    remove(filename);
}
