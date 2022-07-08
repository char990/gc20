#pragma once
#include <vector>
#include <stdint.h>
#include <3rdparty/EasyBmp/EasyBMP.h>

class FrameImage
{
    public:
        void SetId(uint8_t signId, uint8_t slaveFrmId);
        void FillCore(uint8_t * frame);
        std::vector<char> & Save2Base64();

    private:
        BMP bmp;
        uint8_t signId;     // 1 - Max Sign
        uint8_t slaveFrmId;    // 0 - 6
        std::vector<char> base64Img;
        bool newImg{true};
};
