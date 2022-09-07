#include <sign/FrameImage.h>
#include <uci/DbHelper.h>
#include <3rdparty/mongoose/mongoose.h>
#include <tsisp003/TsiSp003Const.h>
#include <module/Utils.h>

using namespace std;
using namespace Utils;

const char *annulus_on = "config/annulus_on.bmp";
const char *annulus_off = "config/annulus_off.bmp";
const char *lantern_on = "config/lantern_on.bmp";
const char *lantern_off = "config/lantern_off.bmp";
const char *islus_sp_frm = "config/islus_%03d.bmp";
const char *uci_frame = "/tmp/uci_frame.bmp";
const char *tmp_sign_frm = "/tmp/Sign%d_Frm%d.bmp";

void FrameImage::SetId(uint8_t signId, uint8_t frmId)
{
    this->signId = signId;
    this->frmId = frmId;
}

void FrameImage::SetRGBA(int colour, RGBApixel &rgba)
{
    rgba.Alpha = 0;
    rgba.Red = (colour >> 16) & 0xFF;
    rgba.Green = (colour >> 8) & 0xFF;
    rgba.Blue = (colour)&0xFF;
}

void FrameImage::FillCore(uint8_t f_colour, uint8_t f_conspicuity, uint8_t *frame)
{ // Both unmapped & mapped colour are allowed as f_colour
    int annulus = (f_conspicuity >> 3) & 0x03;
    if (annulus == 3)
        annulus = 0;
    ReadFromFile(annulus ? annulus_on : annulus_off);
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
    {
        RGBApixel rgba;
        SetRGBA(FrameColour::GetRGB8(prod.GetMappedColour(f_colour)), rgba);
        uint8_t *p = frame;
        int bitOffset = 0;
        for (int j = coreOffsetY; j < (coreOffsetY + coreRows); j++)
        {
            for (int i = coreOffsetX; i < (coreOffsetX + coreColumns); i++)
            {
                if (BitOffset::Get70Bit(p, bitOffset))
                {
                    bmp.SetPixel(i, j, rgba);
                }
                bitOffset++;
            }
        }
    }
    else if (f_colour == 0x0D)
    {
        RGBApixel rgba[10];
        SetRGBA(0, rgba[0]);
        for (int i = 1; i < 10; i++)
        {
            SetRGBA(FrameColour::GetRGB8(prod.GetMappedColour(i)), rgba[i]);
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
                    b >>= 4;
                }
                b &= 0x0F;
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
    if (signId == 0)
    {
        throw invalid_argument("FrameImage::FillCoreFromSlaveFrame signId IS 0");
    }
    if (frmId == 0)
    {
        return;
    }
    newImg = true;
    char filename[256];
    sprintf(filename, tmp_sign_frm, signId, frmId);
    if (frame[1] == 0xFC)
    {
        if (DbHelper::Instance().GetUciProd().IsIslusSpFrm(frame[6]))
        {
            char islus_xxx[256];
            sprintf(islus_xxx, islus_sp_frm, frame[6]);
            ReadFromFile(islus_xxx);
        }
        else
        {
            ReadFromFile(annulus_off);
        }
        WriteToFile(filename);
        return;
    }
    if (frame[1] != 0x0B)
    {
        ReadFromFile(annulus_off);
        return;
    }
    FillCore(frame[6], frame[7], frame + 10);
    WriteToFile(filename);
}

void FrameImage::FillCoreFromUciFrame()
{
    if (signId != 0)
    {
        throw invalid_argument("FrameImage::FillCoreFromUciFrame signId NOT 0");
    }
    newImg = true;
    const char *filename = uci_frame;
    if (DbHelper::Instance().GetUciProd().ProdType() == PRODUCT::ISLUS &&
        DbHelper::Instance().GetUciProd().IsIslusSpFrm(frmId))
    {
        char islus_xxx[256];
        sprintf(islus_xxx, islus_sp_frm, frmId);
        ReadFromFile(islus_xxx);
        WriteToFile(filename);
        return;
    }
    auto frm = DbHelper::Instance().GetUciFrm().GetFrm(frmId);
    if (frm == nullptr)
    {
        return;
    }
    FillCore(frm->colour, frm->conspicuity, frm->stFrm.rawData.data() + frm->frmOffset);
    WriteToFile(filename);
}

void FrameImage::LoadBmpFromBase64(const char *buf, int len)
{
    vector<char> bmpbuf(len / 4 * 3);
    int blen = mg_base64_encode((const unsigned char *)buf, len, bmpbuf.data());

    int fd = open(uci_frame, O_WRONLY);
    if (fd < 0)
    {
        throw invalid_argument(StrFn::PrintfStr("Can't open %s", uci_frame));
    }
    write(fd, bmpbuf.data(), blen);
    close(fd);

    ReadFromFile(uci_frame);

    newImg = false;
    base64Img.resize(len + 1);
    memcpy(base64Img.data(), buf, len);
    base64Img.back() = '\0';
}

std::vector<char> &FrameImage::Save2Base64()
{
    if (newImg)
    {
        char filename[256];
        if (frmId == 0)
        {
            strcpy(filename, annulus_off);
        }
        else
        {
            if (signId == 0)
            {
                strcpy(filename, uci_frame);
            }
            else
            {
                sprintf(filename, tmp_sign_frm, signId, frmId);
            }
        }

        if (Cnvt::File2Base64(filename, base64Img) == 0)
        {
            newImg = false;
        }
    }
    return base64Img;
}

bool FrameImage::ReadFromFile(const char *filename)
{
    if (bmp.ReadFromFile(filename) == false)
    {
        throw runtime_error(StrFn::PrintfStr("Read file error: %s", filename));
    }
    return true;
}

bool FrameImage::WriteToFile(const char *filename)
{
    if (bmp.WriteToFile(filename) == false)
    {
        throw runtime_error(StrFn::PrintfStr("Write file error: %s", filename));
    }
    return true;
}
