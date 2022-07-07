#pragma once
#include <vector>
#include <stdint.h>
#include <3rdparty/EasyBmp/EasyBMP.h>

class FrameImage
{
    public:
        FrameImage(uint8_t signId, uint8_t frameId);
        void FillCore(uint8_t * frame, int len);
        void Save2Base64(std::vector<char> & base64);

    private:
        BMP bmp;
        uint8_t signId;     // 1 - Max Sign
        uint8_t frameId;    // 1 - 6
};
