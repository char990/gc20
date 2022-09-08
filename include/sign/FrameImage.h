#pragma once
#include <vector>
#include <stdint.h>
#include <3rdparty/EasyBmp/EasyBMP.h>

class FrameImage
{
public:
    FrameImage();
    ~FrameImage();
    void SetId(uint8_t signId, uint8_t frmId);
    void FillCoreFromSlaveFrame(uint8_t *frame);
    void FillCoreFromUciFrame();
    void LoadBmpFromBase64(const char * base64, int len);
    std::vector<char> &Save2Base64();

    BMP & GetBmp(){ return bmpSign;};

private:
    BMP bmpSign;
    BMP bmpLanternOn;
    BMP bmpLanternOff;
    BMP bmpLanternFlash;
    uint8_t signId{0}; // Slave: 1 - Max Sign, UciFrame:0
    uint8_t frmId{0};  // Slave: 0 - 6, UciFrame: 1 - 255
    std::vector<char> base64Img;
    bool newImg{true};
    void SetRGBA(int colour, RGBApixel &rgba);

    void FillCore(uint8_t f_colour, uint8_t f_conspicuity, uint8_t *frame);

    void BmpMask(BMP * mask, BMP * base, int x, int y);

    bool ReadFromFile(const char *);
    bool WriteToFile(const char *);
};

